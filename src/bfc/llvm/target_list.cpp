#include "bfc/llvm/target_list.hpp"

#include <array>
#include <fstream>
#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FileUtilities.h>
#include <llvm/Support/Program.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>

namespace bfc::llvm {
namespace {

constexpr std::array<std::string_view, 6> native_targets {
    "x86_64-unknown-linux-gnu", "x86_64-pc-linux-gnu",    "x86_64-unknown-linux-musl",
    "x86_64-w64-windows-gnu",   "x86_64-pc-windows-msvc", "x86_64-pc-windows-itanium",
};

std::optional<std::string> read_clang_targets(std::ostream& error) {
    const auto clang = ::llvm::sys::findProgramByName("clang");
    if (!clang) {
        return std::nullopt;
    }

    ::llvm::SmallString<128> output_path;
    if (const auto file_error = ::llvm::sys::fs::createTemporaryFile("bfc-clang-targets", "txt", output_path)) {
        error << "Warning: could not create temporary file for clang targets: " << file_error.message() << '\n';
        return std::nullopt;
    }
    const ::llvm::FileRemover remove_output(output_path);

    const ::llvm::SmallVector<::llvm::StringRef, 2> arguments {*clang, "--print-targets"};
    const std::array<std::optional<::llvm::StringRef>, 3> redirects {
        std::nullopt,
        output_path,
        std::nullopt,
    };
    std::string execution_error;
    bool execution_failed = false;
    const int exit_code = ::llvm::sys::ExecuteAndWait(*clang, arguments, std::nullopt, redirects, 0, 0,
                                                      &execution_error, &execution_failed);
    if (execution_failed || exit_code != 0) {
        error << "Warning: could not query clang targets";
        if (!execution_error.empty()) {
            error << ": " << execution_error;
        } else {
            error << ": clang exited with code " << exit_code;
        }
        error << '\n';
        return std::nullopt;
    }

    std::ifstream clang_output(output_path.c_str());
    if (!clang_output) {
        error << "Warning: could not read clang target list\n";
        return std::nullopt;
    }

    std::ostringstream contents;
    contents << clang_output.rdbuf();
    if (clang_output.bad()) {
        error << "Warning: could not read clang target list\n";
        return std::nullopt;
    }
    return contents.str();
}

void write_native_targets(std::ostream& output) {
    const std::string default_target = ::llvm::Triple::normalize(::llvm::sys::getDefaultTargetTriple());

    output << "Native targets:\n";
    for (const std::string_view target : native_targets) {
        output << "  " << target;
        if (::llvm::Triple::normalize(target) == default_target) {
            output << " (default)";
        }
        output << '\n';
    }
}

} // namespace

void write_target_list(std::ostream& output, std::ostream& error) {
    write_native_targets(output);

    const auto clang_targets = read_clang_targets(error);
    if (!clang_targets.has_value() || clang_targets->empty()) {
        return;
    }

    std::string_view targets = *clang_targets;
    while (!targets.empty() && (targets.front() == '\n' || targets.front() == '\r')) {
        targets.remove_prefix(1);
    }

    output << "\nExternal targets through clang:\n" << targets;
    if (!targets.ends_with('\n')) {
        output << '\n';
    }
}

} // namespace bfc::llvm
