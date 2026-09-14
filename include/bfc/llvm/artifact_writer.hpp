#pragma once

#include <iosfwd>

namespace llvm {
class Module;
}

namespace bfc::llvm {

class ArtifactWriter {
  public:
    virtual ~ArtifactWriter() = default;

    virtual void write(::llvm::Module& module, std::ostream& output) const = 0;
};

} // namespace bfc::llvm
