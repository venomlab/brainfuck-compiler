#pragma once

#include <CLI/CLI.hpp>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace bfc::cli {

enum class OutputFormat : std::uint8_t {
    IR,
    Assembly,
    Object,
    Executable,
};

struct Options {
    OutputFormat output_format;
    std::optional<std::filesystem::path> input_path;
    std::optional<std::filesystem::path> output_path;
    std::optional<std::string> target_triple;
};

class CommandLine {
  public:
    CommandLine();

    [[nodiscard]] Options parse(int argc, char* argv[]);
    [[nodiscard]] int exit(const CLI::ParseError& error) const;

  private:
    CLI::App app_;
    bool ir_ = false;
    bool assembly_ = false;
    bool object_ = false;
    bool executable_ = false;
    std::string output_path_;
    std::string target_triple_;
    std::string input_path_;
};

} // namespace bfc::cli
