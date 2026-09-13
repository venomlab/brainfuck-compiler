#include "bfc/core/lexer.hpp"
#include "bfc/core/parser.hpp"
#include "bfc/llvm/assembly_writer.hpp"
#include "bfc/llvm/ir_generator.hpp"
#include "bfc/llvm/ir_writer.hpp"

#include <iostream>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>
#include <string_view>

int main(const int argc, char* argv[]) {
    const std::string_view output_type = argc == 2 ? argv[1] : "--ir";
    if (argc > 2 || (output_type != "--ir" && output_type != "--asm")) {
        std::cerr << "Usage: bfc [--ir|--asm]\n";
        return 1;
    }

    bfc::lexer::Tokenizer tokenizer(std::cin);
    bfc::parser::Parser parser(tokenizer);
    auto program = parser.parse();

    llvm::LLVMContext context;
    llvm::Module module("brainfuck", context);
    bfc::llvm::IRGenerator generator(module);
    generator.generate(*program);

    if (output_type == "--asm") {
        const bfc::llvm::AssemblyWriter writer {llvm::Triple(llvm::sys::getDefaultTargetTriple())};
        writer.write(module, std::cout);
    } else {
        const bfc::llvm::IRWriter writer;
        writer.write(module, std::cout);
    }
}
