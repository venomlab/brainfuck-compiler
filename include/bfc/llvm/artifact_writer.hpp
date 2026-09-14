#pragma once

#include <iosfwd>
#include <string_view>

namespace llvm {
class Module;
}

namespace bfc::llvm {

class ArtifactWriter {
  public:
    virtual ~ArtifactWriter() = default;

    [[nodiscard]] virtual std::string_view file_ext() const = 0;
    virtual void write(::llvm::Module& module, std::ostream& output) const = 0;
};

} // namespace bfc::llvm
