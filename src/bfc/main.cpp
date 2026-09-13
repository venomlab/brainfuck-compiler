#include "bfc/core/lexer.hpp"
#include "bfc/core/parser.hpp"
#include "bfc/llvm/ir_generator.hpp"
#include "bfc/llvm/ir_writer.hpp"

#include <iostream>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

int main() {
    bfc::lexer::Tokenizer tokenizer(std::cin);
    bfc::parser::Parser parser(tokenizer);
    auto program = parser.parse();

    llvm::LLVMContext context;
    llvm::Module module("brainfuck", context);
    bfc::llvm::IRGenerator generator(module);
    generator.generate(*program);

    const bfc::llvm::IRWriter writer;
    writer.write(module, std::cout);
}
