#include <bfc/core/ast.hpp>
#include <bfc/llvm/ir_generator.hpp>
#include <bfc/llvm/native_emission_exception.hpp>
#include <bfc/llvm/object_writer.hpp>
#include <gtest/gtest.h>
#include <ios>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/MemoryBufferRef.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace bfc::llvm {
namespace {

ast::Program make_program() {
    std::vector<std::unique_ptr<ast::Node>> operations;
    operations.push_back(std::make_unique<ast::Inc>());
    return ast::Program(std::make_unique<ast::Sequence>(std::move(operations)));
}

TEST(ObjectWriterTest, WritesObjectForTarget) {
    const auto program = make_program();
    ::llvm::LLVMContext context;
    ::llvm::Module module("brainfuck", context);
    IRGenerator generator(module);
    generator.generate(program);

    const ::llvm::Triple target(::llvm::sys::getDefaultTargetTriple());
    const ObjectWriter writer(target);
    std::ostringstream output(std::ios::out | std::ios::binary);

    writer.write(module, output);

    const std::string object_data = output.str();
    ASSERT_FALSE(object_data.empty());
    EXPECT_EQ(module.getTargetTriple(), target.str());
    EXPECT_FALSE(module.getDataLayoutStr().empty());

    const ::llvm::MemoryBufferRef buffer(::llvm::StringRef(object_data.data(), object_data.size()), "brainfuck.o");
    auto object = ::llvm::object::ObjectFile::createObjectFile(buffer);
    if (!object) {
        FAIL() << ::llvm::toString(object.takeError());
    }

    bool has_main = false;
    for (const auto& symbol : (*object)->symbols()) {
        auto name = symbol.getName();
        if (!name) {
            FAIL() << ::llvm::toString(name.takeError());
        }
        if (*name == "main" || *name == "_main") {
            has_main = true;
        }
    }
    EXPECT_TRUE(has_main);
}

TEST(ObjectWriterTest, RejectsBrokenOutputStream) {
    const auto program = make_program();
    ::llvm::LLVMContext context;
    ::llvm::Module module("brainfuck", context);
    IRGenerator generator(module);
    generator.generate(program);

    const ObjectWriter writer {::llvm::Triple(::llvm::sys::getDefaultTargetTriple())};
    std::ostringstream output;
    output.setstate(std::ios::badbit);

    EXPECT_THROW(writer.write(module, output), NativeEmissionException);
}

} // namespace
} // namespace bfc::llvm
