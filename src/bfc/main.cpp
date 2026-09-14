#include "bfc/core/lexer.hpp"
#include "bfc/core/parser.hpp"
#include "bfc/llvm/artifact_writer.hpp"
#include "bfc/llvm/assembly_writer.hpp"
#include "bfc/llvm/external_link_writer.hpp"
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
#include <memory>
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

        const llvm::Triple target(llvm::sys::getDefaultTargetTriple());
        std::unique_ptr<bfc::llvm::ArtifactWriter> writer;
        std::filesystem::path output_path;
        if (output_type == "--ir") {
            writer = std::make_unique<bfc::llvm::IRWriter>();
        } else if (output_type == "--asm") {
            writer = std::make_unique<bfc::llvm::AssemblyWriter>(target);
        } else if (output_type == "--obj") {
            writer = std::make_unique<bfc::llvm::ObjectWriter>(target);
            output_path = "out.o";
        } else {
            writer = std::make_unique<bfc::llvm::ExternalLinkWriter>(target, std::make_unique<bfc::llvm::IRWriter>());
            output_path = "a.out";
        }

        std::ofstream output_file;
        std::ostream* output = &std::cout;
        if (!output_path.empty()) {
            output_file.open(output_path, std::ios::binary | std::ios::trunc);
            if (!output_file) {
                throw std::runtime_error("Could not open " + output_path.string());
            }
            output = &output_file;
        }

        writer->write(module, *output);

        if (output_file.is_open()) {
            output_file.close();
            if (!output_file) {
                throw std::runtime_error("Could not close " + output_path.string());
            }
        }

        if (output_type == "--exe") {
            std::filesystem::permissions(output_path,
                                         std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec |
                                             std::filesystem::perms::others_exec,
                                         std::filesystem::perm_options::add);
        }
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
