#pragma once

#include <iosfwd>
#include <llvm/TargetParser/Triple.h>

namespace llvm {
class Module;
} // namespace llvm

namespace bfc::llvm {

class ObjectWriter {
  private:
    ::llvm::Triple target_;

  public:
    explicit ObjectWriter(::llvm::Triple target);

    void write(::llvm::Module& module, std::ostream& output) const;
};

} // namespace bfc::llvm
