#include "bfc/llvm/lld_link_writer.hpp"

#include "bfc/llvm/executable_emission_exception.hpp"

#include <array>
#include <exception>
#include <fstream>
#include <lld/Common/Driver.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/BinaryFormat/COFF.h>
#include <llvm/Object/COFFImportFile.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FileUtilities.h>
#include <llvm/Support/Process.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/TargetParser/Triple.h>
#include <mutex>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

LLD_HAS_DRIVER(elf)
LLD_HAS_DRIVER(coff)

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

::llvm::object::COFFShortExport windows_export(const std::string_view name) {
    ::llvm::object::COFFShortExport symbol;
    symbol.Name = name;
    return symbol;
}

void write_windows_import_library(const std::string& path) {
    const std::array exports {
        windows_export("GetStdHandle"),
        windows_export("ReadFile"),
        windows_export("WriteFile"),
        windows_export("ExitProcess"),
    };

    if (auto error = ::llvm::object::writeImportLibrary("KERNEL32.dll", path, exports,
                                                        ::llvm::COFF::IMAGE_FILE_MACHINE_AMD64, false)) {
        throw ExecutableEmissionException("Could not create Windows import library: " +
                                          ::llvm::toString(std::move(error)));
    }
}

void run_lld(const ::llvm::ArrayRef<const char*> arguments, const ::llvm::ArrayRef<::lld::DriverDef> drivers) {
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

void link_elf(const std::string& object_path, const std::string& executable_path) {
    const std::array arguments {
        "ld.lld", "-static", "--entry=_start", "--fatal-warnings", object_path.c_str(), "-o", executable_path.c_str(),
    };
    const std::array drivers {
        ::lld::DriverDef {::lld::Gnu, &::lld::elf::link},
    };

    run_lld(arguments, drivers);
}

void link_coff(const std::string& object_path, const std::string& executable_path) {
    const std::string import_library_path = create_temporary_file("lib");
    const ::llvm::FileRemover remove_import_library(import_library_path);
    write_windows_import_library(import_library_path);

    const std::string output_argument = "/out:" + executable_path;
    const std::array arguments {
        "lld-link",     "/entry:mainCRTStartup", "/subsystem:console",        "/nodefaultlib",
        "/machine:x64", object_path.c_str(),     import_library_path.c_str(), output_argument.c_str(),
    };
    const std::array drivers {
        ::lld::DriverDef {::lld::WinLink, &::lld::coff::link},
    };

    run_lld(arguments, drivers);
}

void link_executable(const ::llvm::Triple& target, const std::string& object_path, const std::string& executable_path) {
    if (target.isOSLinux()) {
        link_elf(object_path, executable_path);
        return;
    }
    if (target.getArch() == ::llvm::Triple::x86_64 && target.isOSWindows()) {
        link_coff(object_path, executable_path);
        return;
    }

    throw ExecutableEmissionException("Unsupported LLD executable target: " + target.str());
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
    const ::llvm::Triple& target = object_writer_.target();
    const bool windows = target.isOSWindows();
    const std::string object_path = create_temporary_file(windows ? "obj" : "o");
    const ::llvm::FileRemover remove_object(object_path);

    const std::string executable_path = create_temporary_file(windows ? "exe" : "out");
    const ::llvm::FileRemover remove_executable(executable_path);

    write_object(object_writer_, module, object_path);
    link_executable(target, object_path, executable_path);
    copy_executable(executable_path, output);
}

} // namespace bfc::llvm
