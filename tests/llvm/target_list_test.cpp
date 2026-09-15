#include <array>
#include <bfc/llvm/target_list.hpp>
#include <gtest/gtest.h>
#include <llvm/Support/Program.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>
#include <sstream>
#include <string>
#include <string_view>

namespace bfc::llvm {
namespace {

constexpr std::array<std::string_view, 6> native_targets {
    "x86_64-unknown-linux-gnu", "x86_64-pc-linux-gnu",    "x86_64-unknown-linux-musl",
    "x86_64-w64-windows-gnu",   "x86_64-pc-windows-msvc", "x86_64-pc-windows-itanium",
};

TEST(TargetListTest, WritesEveryNativeTarget) {
    std::ostringstream output;
    std::ostringstream error;

    write_target_list(output, error);

    EXPECT_TRUE(output.str().starts_with("Native targets:\n"));
    for (const std::string_view target : native_targets) {
        EXPECT_NE(output.str().find(target), std::string::npos);
    }
}

TEST(TargetListTest, MarksDefaultNativeTarget) {
    std::ostringstream output;
    std::ostringstream error;

    write_target_list(output, error);

    const std::string default_target = ::llvm::Triple::normalize(::llvm::sys::getDefaultTargetTriple());
    bool default_is_native = false;
    for (const std::string_view target : native_targets) {
        if (::llvm::Triple::normalize(target) == default_target) {
            default_is_native = true;
            EXPECT_NE(output.str().find(std::string(target) + " (default)"), std::string::npos);
        }
    }
    if (!default_is_native) {
        EXPECT_EQ(output.str().find(" (default)"), std::string::npos);
    }
}

TEST(TargetListTest, WritesTargetsReportedByClangWhenAvailable) {
    if (!::llvm::sys::findProgramByName("clang")) {
        GTEST_SKIP() << "clang is not available";
    }

    std::ostringstream output;
    std::ostringstream error;

    write_target_list(output, error);

    EXPECT_NE(output.str().find("External targets through clang:\n"), std::string::npos);
    EXPECT_NE(output.str().find("Registered Targets:"), std::string::npos);
    EXPECT_TRUE(error.str().empty());
}

} // namespace
} // namespace bfc::llvm
