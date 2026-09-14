#include <bfc/llvm/ir_writer.hpp>
#include <gtest/gtest.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <sstream>
#include <string>

namespace bfc::llvm {
namespace {

TEST(IRWriterTest, WritesModuleToStandardOutputStream) {
    ::llvm::LLVMContext context;
    ::llvm::Module module {"brainfuck", context};
    std::ostringstream output;
    const IRWriter writer;

    writer.write(module, output);

    EXPECT_NE(output.str().find("; ModuleID = 'brainfuck'"), std::string::npos);
}

} // namespace
} // namespace bfc::llvm
