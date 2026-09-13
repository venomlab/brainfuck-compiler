#pragma once

#include <iosfwd>

namespace llvm {
class Module;
}

namespace bfc::llvm {

class IRWriter {
  public:
    void write(const ::llvm::Module& module, std::ostream& output) const;
};

} // namespace bfc::llvm
