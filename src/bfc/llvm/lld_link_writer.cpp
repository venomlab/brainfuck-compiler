#include "bfc/llvm/lld_link_writer.hpp"

#include "bfc/llvm/executable_emission_exception.hpp"

#include <array>
#include <exception>
#include <fstream>
#include <lld/Common/Driver.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FileUtilities.h>
#include <llvm/Support/Process.h>
#include <llvm/Support/raw_ostream.h>
#include <mutex>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

LLD_HAS_DRIVER(elf)

namespace bfc::llvm {
namespace {

std::mutex lld_mutex;
bool can_run_lld = true;

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
        output.close();
        if (!output) {
            throw ExecutableEmissionException("Could not close temporary object file");
        }
    } catch (const std::exception& error) {
        throw ExecutableEmissionException("Could not emit object file: " + std::string(error.what()));
    }
}

void link_executable(const std::string& object_path, const std::string& executable_path) {
    const std::array arguments {
        "ld.lld", "-static", "--entry=_start", "--fatal-warnings", object_path.c_str(), "-o", executable_path.c_str(),
    };
    const std::array drivers {
        ::lld::DriverDef {::lld::Gnu, &::lld::elf::link},
    };

    std::string stdout_message;
    std::string stderr_message;
    ::llvm::raw_string_ostream stdout_stream(stdout_message);
    ::llvm::raw_string_ostream stderr_stream(stderr_message);

    std::lock_guard lock(lld_mutex);
    if (!can_run_lld) {
        throw ExecutableEmissionException("LLD cannot run again after a previous failure");
    }

    const ::lld::Result result = ::lld::lldMain(arguments, stdout_stream, stderr_stream, drivers);
    can_run_lld = result.canRunAgain;
    stdout_stream.flush();
    stderr_stream.flush();

    if (!result.canRunAgain) {
        throw ExecutableEmissionException("LLD cannot run again: " + stderr_message);
    }
    if (result.retCode != 0) {
        const std::string& message = stderr_message.empty() ? stdout_message : stderr_message;
        throw ExecutableEmissionException("LLD exited with code " + std::to_string(result.retCode) + ": " + message);
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

LLDLinkWriter::LLDLinkWriter(ObjectWriter object_writer) : object_writer_(std::move(object_writer)) {}

std::string_view LLDLinkWriter::file_ext() const {
    return {};
}

void LLDLinkWriter::write(::llvm::Module& module, std::ostream& output) const {
    const std::string object_path = create_temporary_file("o");
    const ::llvm::FileRemover remove_object(object_path);

    const std::string executable_path = create_temporary_file("out");
    const ::llvm::FileRemover remove_executable(executable_path);

    write_object(object_writer_, module, object_path);
    link_executable(object_path, executable_path);
    copy_executable(executable_path, output);
}

} // namespace bfc::llvm
