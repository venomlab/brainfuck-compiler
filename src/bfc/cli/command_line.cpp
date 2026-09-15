#include <bfc/cli/command_line.hpp>
#include <string_view>

namespace bfc::cli {
namespace {

std::optional<OutputFormat> explicit_format(bool ir, bool assembly, bool object, bool executable) {
    if (ir) {
        return OutputFormat::IR;
    }
    if (assembly) {
        return OutputFormat::Assembly;
    }
    if (object) {
        return OutputFormat::Object;
    }
    if (executable) {
        return OutputFormat::Executable;
    }
    return std::nullopt;
}

OutputFormat format_from_path(const std::filesystem::path& path) {
    const std::string extension = path.extension().string();

    if (extension == ".ll") {
        return OutputFormat::IR;
    }
    if (extension == ".s" || extension == ".asm") {
        return OutputFormat::Assembly;
    }
    if (extension == ".o" || extension == ".obj") {
        return OutputFormat::Object;
    }
    if (extension.empty() || extension == ".out" || extension == ".exe") {
        return OutputFormat::Executable;
    }

    throw CLI::ValidationError("-o", "unknown output extension: " + extension);
}

} // namespace

CommandLine::CommandLine() : app_("Brainfuck compiler") {
    app_.set_version_flag("--version", "bfc " BFC_VERSION);
    app_.add_flag("--list-targets", list_targets_, "List compilation targets");

    CLI::App* output_format = app_.add_option_group("Output format");
    output_format->add_flag("--ir", ir_, "Emit LLVM IR");
    output_format->add_flag("--asm", assembly_, "Emit assembly");
    output_format->add_flag("--obj", object_, "Emit object file");
    output_format->add_flag("--exe", executable_, "Emit executable");
    output_format->require_option(0, 1);

    app_.add_option("-o", output_path_, "Output file");
    app_.add_option("-t,--target", target_triple_, "Target triple");
    app_.add_option("program", input_path_, "Brainfuck program file");
}

Options CommandLine::parse(int argc, char* argv[]) {
    app_.parse(argc, argv);

    if (list_targets_ && (ir_ || assembly_ || object_ || executable_ || !output_path_.empty() ||
                          !target_triple_.empty() || !input_path_.empty())) {
        throw CLI::ValidationError("--list-targets", "cannot be combined with compilation options");
    }

    const std::optional<OutputFormat> requested = explicit_format(ir_, assembly_, object_, executable_);
    const std::optional<std::filesystem::path> output =
        output_path_.empty() ? std::nullopt : std::make_optional(output_path_);

    OutputFormat format = requested.value_or(OutputFormat::Executable);
    if (output.has_value()) {
        const OutputFormat inferred = format_from_path(*output);
        if (requested.has_value() && *requested != inferred) {
            throw CLI::ValidationError("-o", "output extension conflicts with requested format");
        }
        format = inferred;
    }

    std::optional<std::filesystem::path> input;
    if (!input_path_.empty() && input_path_ != "-") {
        input = input_path_;
    }

    const std::optional<std::string> target =
        target_triple_.empty() ? std::nullopt : std::make_optional(target_triple_);

    return Options {
        .output_format = format,
        .input_path = std::move(input),
        .output_path = output,
        .target_triple = target,
        .list_targets = list_targets_,
    };
}

int CommandLine::exit(const CLI::ParseError& error) const {
    return app_.exit(error);
}

} // namespace bfc::cli
