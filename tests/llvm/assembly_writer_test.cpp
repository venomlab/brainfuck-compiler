#include <bfc/core/ast.hpp>
#include <bfc/llvm/assembly_writer.hpp>
#include <bfc/llvm/ir_generator.hpp>
#include <bfc/llvm/native_emission_exception.hpp>
#include <gtest/gtest.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace bfc::llvm {
namespace {

ast::Program make_empty_program() {
    std::vector<std::unique_ptr<ast::Node>> operations;
    return ast::Program(std::make_unique<ast::Sequence>(std::move(operations)));
}

TEST(AssemblyWriterTest, WritesAssemblyForTarget) {
    const auto program = make_empty_program();
    ::llvm::LLVMContext context;
    ::llvm::Module module("brainfuck", context);
    IRGenerator generator(module);
    generator.generate(program);

    const ::llvm::Triple target(::llvm::sys::getDefaultTargetTriple());
    const AssemblyWriter writer(target);
    std::ostringstream output;

    writer.write(module, output);

    EXPECT_FALSE(output.str().empty());
    EXPECT_NE(output.str().find("main"), std::string::npos);
    EXPECT_EQ(module.getTargetTriple(), target.str());
    EXPECT_FALSE(module.getDataLayoutStr().empty());
}

TEST(AssemblyWriterTest, RejectsUnknownTarget) {
    ::llvm::LLVMContext context;
    ::llvm::Module module("brainfuck", context);
    const AssemblyWriter writer(::llvm::Triple("not-a-real-target"));
    std::ostringstream output;

    EXPECT_THROW(writer.write(module, output), NativeEmissionException);
}

} // namespace
} // namespace bfc::llvm
