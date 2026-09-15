#include <algorithm>
#include <bfc/core/ast.hpp>
#include <bfc/llvm/ir_generator.hpp>
#include <bfc/llvm/object_writer.hpp>
#include <bfc/llvm/runtime_generator.hpp>
#include <gtest/gtest.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/MemoryBufferRef.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/TargetParser/Triple.h>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace bfc::llvm {
namespace {

ast::Program make_io_program() {
    std::vector<std::unique_ptr<ast::Node>> operations;
    operations.push_back(std::make_unique<ast::Read>());
    operations.push_back(std::make_unique<ast::Print>());
    return ast::Program(std::make_unique<ast::Sequence>(std::move(operations)));
}

std::unique_ptr<::llvm::Module> make_module(::llvm::LLVMContext& context) {
    auto program = make_io_program();
    auto module = std::make_unique<::llvm::Module>("brainfuck", context);
    IRGenerator generator(*module);
    generator.generate(program);
    return module;
}

TEST(RuntimeGeneratorTest, SupportsBundledRuntimes) {
    EXPECT_TRUE(RuntimeGenerator::supports(::llvm::Triple("x86_64-unknown-linux-gnu")));
    EXPECT_TRUE(RuntimeGenerator::supports(::llvm::Triple("x86_64-unknown-linux-musl")));
    EXPECT_TRUE(RuntimeGenerator::supports(::llvm::Triple("x86_64-w64-windows-gnu")));
    EXPECT_TRUE(RuntimeGenerator::supports(::llvm::Triple("x86_64-pc-windows-msvc")));
    EXPECT_TRUE(RuntimeGenerator::supports(::llvm::Triple("x86_64-pc-windows-itanium")));
    EXPECT_FALSE(RuntimeGenerator::supports(::llvm::Triple("aarch64-unknown-linux-gnu")));
    EXPECT_FALSE(RuntimeGenerator::supports(::llvm::Triple("aarch64-pc-windows-msvc")));
    EXPECT_FALSE(RuntimeGenerator::supports(::llvm::Triple("x86_64-unknown-linux-gnux32")));
}

TEST(RuntimeGeneratorTest, RejectsUnsupportedTarget) {
    ::llvm::LLVMContext context;
    auto module = make_module(context);
    const RuntimeGenerator generator(::llvm::Triple("aarch64-unknown-linux-gnu"));

    EXPECT_THROW(generator.generate(*module), std::invalid_argument);
}

TEST(RuntimeGeneratorTest, AddsRuntimeToModule) {
    ::llvm::LLVMContext context;
    auto module = make_module(context);
    const ::llvm::Triple target("x86_64-unknown-linux-gnu");
    const RuntimeGenerator generator(target);

    generator.generate(*module);

    EXPECT_EQ(module->getTargetTriple(), target.str());

    const auto* getchar = module->getFunction("getchar");
    ASSERT_NE(getchar, nullptr);
    EXPECT_FALSE(getchar->isDeclaration());

    const auto* putchar = module->getFunction("putchar");
    ASSERT_NE(putchar, nullptr);
    EXPECT_FALSE(putchar->isDeclaration());

    const auto* start = module->getFunction("_start");
    ASSERT_NE(start, nullptr);
    EXPECT_FALSE(start->isDeclaration());
    EXPECT_TRUE(start->hasFnAttribute(::llvm::Attribute::NoReturn));
    EXPECT_TRUE(start->hasFnAttribute("stackrealign"));

    std::string error;
    ::llvm::raw_string_ostream output(error);
    const bool invalid = ::llvm::verifyModule(*module, &output);
    output.flush();

    EXPECT_FALSE(invalid) << error;
}

TEST(RuntimeGeneratorTest, AddsWindowsRuntimeToModule) {
    ::llvm::LLVMContext context;
    auto module = make_module(context);
    const ::llvm::Triple target("x86_64-w64-windows-gnu");
    const RuntimeGenerator generator(target);

    generator.generate(*module);

    EXPECT_EQ(module->getTargetTriple(), target.str());

    for (const std::string_view name : {"getchar", "putchar", "mainCRTStartup"}) {
        const auto* function = module->getFunction(name);
        ASSERT_NE(function, nullptr);
        EXPECT_FALSE(function->isDeclaration());
    }

    const auto* mingw_initializer = module->getFunction("__main");
    ASSERT_NE(mingw_initializer, nullptr);
    EXPECT_FALSE(mingw_initializer->isDeclaration());

    const auto* start = module->getFunction("mainCRTStartup");
    ASSERT_NE(start, nullptr);
    EXPECT_TRUE(start->hasFnAttribute(::llvm::Attribute::NoReturn));

    for (const std::string_view name : {"GetStdHandle", "ReadFile", "WriteFile", "ExitProcess"}) {
        const auto* function = module->getFunction(name);
        ASSERT_NE(function, nullptr);
        EXPECT_TRUE(function->isDeclaration());
    }

    const auto* exit_process = module->getFunction("ExitProcess");
    ASSERT_NE(exit_process, nullptr);
    EXPECT_TRUE(exit_process->hasFnAttribute(::llvm::Attribute::NoReturn));

    std::string error;
    ::llvm::raw_string_ostream output(error);
    const bool invalid = ::llvm::verifyModule(*module, &output);
    output.flush();

    EXPECT_FALSE(invalid) << error;
}

TEST(RuntimeGeneratorTest, OmitsMinGWInitializerForMsvcEnvironment) {
    ::llvm::LLVMContext context;
    auto module = make_module(context);
    const ::llvm::Triple target("x86_64-pc-windows-msvc");

    RuntimeGenerator(target).generate(*module);

    EXPECT_EQ(module->getFunction("__main"), nullptr);
}

TEST(RuntimeGeneratorTest, EmitsRuntimeSymbolsToObject) {
    ::llvm::LLVMContext context;
    auto module = make_module(context);
    const ::llvm::Triple target("x86_64-unknown-linux-gnu");
    RuntimeGenerator(target).generate(*module);

    std::ostringstream output(std::ios::out | std::ios::binary);
    ObjectWriter(target).write(*module, output);

    const std::string object_data = output.str();
    ASSERT_FALSE(object_data.empty());
    const ::llvm::MemoryBufferRef buffer(::llvm::StringRef(object_data.data(), object_data.size()), "brainfuck.o");
    auto object = ::llvm::object::ObjectFile::createObjectFile(buffer);
    if (!object) {
        FAIL() << ::llvm::toString(object.takeError());
    }

    std::vector<std::string> symbols;
    for (const auto& symbol : (*object)->symbols()) {
        auto name = symbol.getName();
        if (!name) {
            FAIL() << ::llvm::toString(name.takeError());
        }
        symbols.emplace_back(*name);
    }

    for (const std::string_view name : {"_start", "getchar", "putchar"}) {
        EXPECT_NE(std::find(symbols.begin(), symbols.end(), name), symbols.end());
    }
}

TEST(RuntimeGeneratorTest, EmitsWindowsRuntimeSymbolsToObject) {
    ::llvm::LLVMContext context;
    auto module = make_module(context);
    const ::llvm::Triple target("x86_64-w64-windows-gnu");
    RuntimeGenerator(target).generate(*module);

    std::ostringstream output(std::ios::out | std::ios::binary);
    ObjectWriter(target).write(*module, output);

    const std::string object_data = output.str();
    ASSERT_FALSE(object_data.empty());
    const ::llvm::MemoryBufferRef buffer(::llvm::StringRef(object_data.data(), object_data.size()), "brainfuck.obj");
    auto object = ::llvm::object::ObjectFile::createObjectFile(buffer);
    if (!object) {
        FAIL() << ::llvm::toString(object.takeError());
    }

    std::vector<std::string> symbols;
    for (const auto& symbol : (*object)->symbols()) {
        auto name = symbol.getName();
        if (!name) {
            FAIL() << ::llvm::toString(name.takeError());
        }
        symbols.emplace_back(*name);
    }

    for (const std::string_view name : {"mainCRTStartup", "getchar", "putchar"}) {
        EXPECT_NE(std::find(symbols.begin(), symbols.end(), name), symbols.end());
    }
}

} // namespace
} // namespace bfc::llvm
