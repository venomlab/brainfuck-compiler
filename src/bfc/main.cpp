#include "bfc/core/lexer.hpp"
#include "bfc/core/parser.hpp"
#include "bfc/llvm/assembly_writer.hpp"
#include "bfc/llvm/executable_writer.hpp"
#include "bfc/llvm/ir_generator.hpp"
#include "bfc/llvm/ir_writer.hpp"
#include "bfc/llvm/object_writer.hpp"

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>
#include <stdexcept>
#include <string_view>

int main(const int argc, char* argv[]) {
    const std::string_view output_type = argc == 2 ? argv[1] : "--exe";
    if (argc > 2 ||
        (output_type != "--ir" && output_type != "--asm" && output_type != "--obj" && output_type != "--exe")) {
        std::cerr << "Usage: bfc [--ir|--asm|--obj|--exe]\n";
        return 1;
    }

    try {
        bfc::lexer::Tokenizer tokenizer(std::cin);
        bfc::parser::Parser parser(tokenizer);
        auto program = parser.parse();

        llvm::LLVMContext context;
        llvm::Module module("brainfuck", context);
        bfc::llvm::IRGenerator generator(module);
        generator.generate(*program);

        if (output_type == "--ir") {
            const bfc::llvm::IRWriter writer;
            writer.write(module, std::cout);
        } else if (output_type == "--asm") {
            const bfc::llvm::AssemblyWriter writer {llvm::Triple(llvm::sys::getDefaultTargetTriple())};
            writer.write(module, std::cout);
        } else if (output_type == "--obj") {
            std::ofstream output("out.o", std::ios::binary | std::ios::trunc);
            const bfc::llvm::ObjectWriter writer {llvm::Triple(llvm::sys::getDefaultTargetTriple())};
            writer.write(module, output);
        } else {
            std::ofstream output("a.out", std::ios::binary | std::ios::trunc);
            if (!output) {
                throw std::runtime_error("Could not open a.out");
            }

            const bfc::llvm::ExecutableWriter writer {
                bfc::llvm::ObjectWriter(llvm::Triple(llvm::sys::getDefaultTargetTriple()))};
            writer.write(module, output);

            output.close();
            if (!output) {
                throw std::runtime_error("Could not close a.out");
            }

            std::filesystem::permissions("a.out",
                                         std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec |
                                             std::filesystem::perms::others_exec,
                                         std::filesystem::perm_options::add);
        }
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
