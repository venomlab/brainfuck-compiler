#include <array>
#include <bfc/cli/command_line.hpp>
#include <filesystem>
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace bfc::cli {
namespace {

Options parse(std::initializer_list<std::string> arguments) {
    std::vector<std::string> storage(arguments);
    std::vector<char*> argv;
    argv.reserve(storage.size());
    for (std::string& argument : storage) {
        argv.push_back(argument.data());
    }

    CommandLine command_line;
    return command_line.parse(static_cast<int>(argv.size()), argv.data());
}

TEST(CommandLineTest, UsesExecutableAndStandardStreamsByDefault) {
    const Options options = parse({"bfc"});

    EXPECT_EQ(options.output_format, OutputFormat::Executable);
    EXPECT_FALSE(options.input_path.has_value());
    EXPECT_FALSE(options.output_path.has_value());
    EXPECT_FALSE(options.target_triple.has_value());
}

TEST(CommandLineTest, ReadsProgramFromPath) {
    const Options options = parse({"bfc", "hello.bf"});

    EXPECT_EQ(options.input_path, std::filesystem::path("hello.bf"));
}

TEST(CommandLineTest, TreatsDashProgramAsStandardInput) {
    const Options options = parse({"bfc", "-"});

    EXPECT_FALSE(options.input_path.has_value());
}

TEST(CommandLineTest, ParsesTargetTriple) {
    const Options options = parse({"bfc", "--target", "aarch64-linux-gnu"});

    EXPECT_EQ(options.target_triple, "aarch64-linux-gnu");
}

TEST(CommandLineTest, ParsesShortTargetOption) {
    const Options options = parse({"bfc", "-t", "aarch64-linux-gnu"});

    EXPECT_EQ(options.target_triple, "aarch64-linux-gnu");
}

TEST(CommandLineTest, UsesExplicitOutputFormat) {
    EXPECT_EQ(parse({"bfc", "--ir"}).output_format, OutputFormat::IR);
    EXPECT_EQ(parse({"bfc", "--asm"}).output_format, OutputFormat::Assembly);
    EXPECT_EQ(parse({"bfc", "--obj"}).output_format, OutputFormat::Object);
    EXPECT_EQ(parse({"bfc", "--exe"}).output_format, OutputFormat::Executable);
}

TEST(CommandLineTest, InfersOutputFormatFromPath) {
    const std::array cases {
        std::pair {"program.ll", OutputFormat::IR},          std::pair {"program.s", OutputFormat::Assembly},
        std::pair {"program.asm", OutputFormat::Assembly},   std::pair {"program.o", OutputFormat::Object},
        std::pair {"program.obj", OutputFormat::Object},     std::pair {"program.out", OutputFormat::Executable},
        std::pair {"program.exe", OutputFormat::Executable}, std::pair {"program", OutputFormat::Executable},
    };

    for (const auto& [path, format] : cases) {
        const Options options = parse({"bfc", "-o", path});
        EXPECT_EQ(options.output_format, format) << path;
        EXPECT_EQ(options.output_path, std::filesystem::path(path));
    }
}

TEST(CommandLineTest, RejectsMultipleOutputFormats) {
    EXPECT_THROW(parse({"bfc", "--ir", "--asm"}), CLI::ParseError);
}

TEST(CommandLineTest, RejectsConflictingOutputExtension) {
    EXPECT_THROW(parse({"bfc", "--ir", "-o", "program.o"}), CLI::ValidationError);
}

TEST(CommandLineTest, RejectsUnknownOutputExtension) {
    EXPECT_THROW(parse({"bfc", "-o", "program.txt"}), CLI::ValidationError);
}

TEST(CommandLineTest, RejectsMultipleProgramFiles) {
    EXPECT_THROW(parse({"bfc", "one.bf", "two.bf"}), CLI::ParseError);
}

TEST(CommandLineTest, SupportsHelp) {
    EXPECT_THROW(parse({"bfc", "--help"}), CLI::CallForHelp);
}

} // namespace
} // namespace bfc::cli
