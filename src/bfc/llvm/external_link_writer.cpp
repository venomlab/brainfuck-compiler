#include "bfc/llvm/external_link_writer.hpp"

#include "bfc/llvm/executable_emission_exception.hpp"

#include <exception>
#include <fstream>
#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FileUtilities.h>
#include <llvm/Support/Process.h>
#include <llvm/Support/Program.h>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

namespace bfc::llvm {
namespace {

std::string create_temporary_file(const std::string_view file_ext) {
    const std::string prefix = "brainfuck-compiler-" + std::to_string(::llvm::sys::Process::getProcessId());
    const std::string_view suffix = file_ext.starts_with('.') ? file_ext.substr(1) : file_ext;
    ::llvm::SmallString<128> path;
    if (const auto error = ::llvm::sys::fs::createTemporaryFile(prefix, suffix, path)) {
        throw ExecutableEmissionException("Could not create temporary file: " + error.message());
    }
    return path.str().str();
}

void write_artifact(const ArtifactWriter& writer, ::llvm::Module& module, const std::string& path) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw ExecutableEmissionException("Could not open temporary linker input");
    }

    try {
        writer.write(module, output);
    } catch (const std::exception& error) {
        throw ExecutableEmissionException("Could not emit linker input: " + std::string(error.what()));
    }
}

void link_executable(const ::llvm::Triple& target, const std::string& artifact_path,
                     const std::string& executable_path) {
    const auto clang = ::llvm::sys::findProgramByName("clang");
    if (!clang) {
        throw ExecutableEmissionException("Could not find clang: " + clang.getError().message());
    }

    const std::string target_argument = "--target=" + target.str();
    const ::llvm::SmallVector<::llvm::StringRef, 6> arguments {
        *clang, target_argument, "-static", artifact_path, "-o", executable_path,
    };

    std::string error;
    bool execution_failed = false;
    const int exit_code =
        ::llvm::sys::ExecuteAndWait(*clang, arguments, std::nullopt, {}, 0, 0, &error, &execution_failed);
    if (execution_failed) {
        throw ExecutableEmissionException("Could not execute clang: " + error);
    }
    if (exit_code != 0) {
        throw ExecutableEmissionException("clang exited with code " + std::to_string(exit_code));
    }
}

void copy_executable(const std::string& path, std::ostream& output) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw ExecutableEmissionException("Could not open linked executable");
    }

    output << input.rdbuf();
    if (input.bad() || !output) {
        throw ExecutableEmissionException("Could not write executable output");
    }
}

} // namespace

ExternalLinkWriter::ExternalLinkWriter(::llvm::Triple target, std::unique_ptr<ArtifactWriter> artifact_writer)
    : target_(std::move(target)), artifact_writer_(std::move(artifact_writer)) {}

std::string_view ExternalLinkWriter::file_ext() const {
    return {};
}

void ExternalLinkWriter::write(::llvm::Module& module, std::ostream& output) const {
    module.setTargetTriple(target_.str());

    const std::string artifact_path = create_temporary_file(artifact_writer_->file_ext());
    const ::llvm::FileRemover remove_artifact(artifact_path);

    const std::string executable_path = create_temporary_file(".out");
    const ::llvm::FileRemover remove_executable(executable_path);

    write_artifact(*artifact_writer_, module, artifact_path);
    link_executable(target_, artifact_path, executable_path);
    copy_executable(executable_path, output);
}

} // namespace bfc::llvm
