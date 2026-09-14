#include "bfc/llvm/executable_writer.hpp"

#include "bfc/llvm/executable_emission_exception.hpp"

#include <exception>
#include <fstream>
#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FileUtilities.h>
#include <llvm/Support/Process.h>
#include <llvm/Support/Program.h>
#include <llvm/TargetParser/Triple.h>
#include <optional>
#include <ostream>
#include <string>
#include <utility>

namespace bfc::llvm {
namespace {

std::string create_temporary_file(const ::llvm::StringRef suffix) {
    const std::string prefix = "brainfuck-compiler-" + std::to_string(::llvm::sys::Process::getProcessId());
    ::llvm::SmallString<128> path;
    if (const auto error = ::llvm::sys::fs::createTemporaryFile(prefix, suffix, path)) {
        throw ExecutableEmissionException("Could not create temporary file: " + error.message());
    }
    return path.str().str();
}

void write_object(const ObjectWriter& writer, ::llvm::Module& module, const std::string& path) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw ExecutableEmissionException("Could not open temporary object file");
    }

    try {
        writer.write(module, output);
    } catch (const std::exception& error) {
        throw ExecutableEmissionException("Could not emit object file: " + std::string(error.what()));
    }
}

void link_executable(const ::llvm::Triple& target_triple, const std::string& object_path,
                     const std::string& executable_path) {
    const auto clang = ::llvm::sys::findProgramByName("clang");
    if (!clang) {
        throw ExecutableEmissionException("Could not find clang: " + clang.getError().message());
    }

    const std::string target = "--target=" + target_triple.str();
    const ::llvm::SmallVector<::llvm::StringRef, 6> arguments {
        *clang, target, "-static", object_path, "-o", executable_path,
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

ExecutableWriter::ExecutableWriter(ObjectWriter object_writer) : object_writer_(std::move(object_writer)) {}

void ExecutableWriter::write(::llvm::Module& module, std::ostream& output) const {
    const std::string object_path = create_temporary_file("o");
    const ::llvm::FileRemover remove_object(object_path);

    const std::string executable_path = create_temporary_file("out");
    const ::llvm::FileRemover remove_executable(executable_path);

    write_object(object_writer_, module, object_path);
    link_executable(object_writer_.target(), object_path, executable_path);
    copy_executable(executable_path, output);
}

} // namespace bfc::llvm
