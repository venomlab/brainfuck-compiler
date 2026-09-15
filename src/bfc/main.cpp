#include "bfc/cli/command_line.hpp"
#include "bfc/core/lexer.hpp"
#include "bfc/core/parser.hpp"
#include "bfc/llvm/artifact_writer.hpp"
#include "bfc/llvm/assembly_writer.hpp"
#include "bfc/llvm/external_link_writer.hpp"
#include "bfc/llvm/ir_generator.hpp"
#include "bfc/llvm/ir_writer.hpp"
#include "bfc/llvm/lld_link_writer.hpp"
#include "bfc/llvm/object_writer.hpp"
#include "bfc/llvm/runtime_generator.hpp"
#include "bfc/llvm/target_list.hpp"

#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>
#include <memory>
#include <stdexcept>

namespace {

std::unique_ptr<bfc::llvm::ArtifactWriter> make_writer(const bfc::cli::OutputFormat format, const llvm::Triple& target,
                                                       llvm::Module& module) {
    switch (format) {
    case bfc::cli::OutputFormat::IR:
        return std::make_unique<bfc::llvm::IRWriter>();
    case bfc::cli::OutputFormat::Assembly:
        return std::make_unique<bfc::llvm::AssemblyWriter>(target);
    case bfc::cli::OutputFormat::Object:
        return std::make_unique<bfc::llvm::ObjectWriter>(target);
    case bfc::cli::OutputFormat::Executable:
        if (bfc::llvm::RuntimeGenerator::supports(target)) {
            bfc::llvm::RuntimeGenerator(target).generate(module);
            return std::make_unique<bfc::llvm::LLDLinkWriter>(bfc::llvm::ObjectWriter(target));
        }
        return std::make_unique<bfc::llvm::ExternalLinkWriter>(target, std::make_unique<bfc::llvm::IRWriter>());
    }

    throw std::logic_error("Unknown output format");
}

} // namespace

int main(const int argc, char* argv[]) {
    bfc::cli::CommandLine command_line;

    try {
        const bfc::cli::Options options = command_line.parse(argc, argv);
        if (options.list_targets) {
            bfc::llvm::write_target_list(std::cout, std::cerr);
            return std::cout ? 0 : 1;
        }

        std::ifstream input_file;
        std::istream* input = &std::cin;
        if (options.input_path.has_value()) {
            input_file.open(*options.input_path);
            if (!input_file) {
                throw std::runtime_error("Could not open input file: " + options.input_path->string());
            }
            input = &input_file;
        }

        bfc::lexer::Tokenizer tokenizer(*input);
        bfc::parser::Parser parser(tokenizer);
        const auto program = parser.parse();

        llvm::LLVMContext context;
        llvm::Module module("brainfuck", context);
        bfc::llvm::IRGenerator generator(module);
        generator.generate(*program);

        const llvm::Triple target(options.target_triple.value_or(llvm::sys::getDefaultTargetTriple()));
        module.setTargetTriple(target.str());
        const auto writer = make_writer(options.output_format, target, module);

        std::ofstream output_file;
        std::ostream* output = &std::cout;
        if (options.output_path.has_value()) {
            output_file.open(*options.output_path, std::ios::binary | std::ios::trunc);
            if (!output_file) {
                throw std::runtime_error("Could not open output file: " + options.output_path->string());
            }
            output = &output_file;
        }

        writer->write(module, *output);
        output->flush();
        if (!*output) {
            throw std::runtime_error("Could not write output");
        }

        if (options.output_path.has_value()) {
            output_file.close();
            if (!output_file) {
                auto error_msg = std::format("Could not close output file: {}", options.output_path->string());
                throw std::runtime_error(error_msg);
            }
        }

        if (options.output_format == bfc::cli::OutputFormat::Executable && options.output_path.has_value()) {
            std::filesystem::permissions(*options.output_path,
                                         std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec |
                                             std::filesystem::perms::others_exec,
                                         std::filesystem::perm_options::add);
        }
    } catch (const CLI::ParseError& error) {
        return command_line.exit(error);
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
