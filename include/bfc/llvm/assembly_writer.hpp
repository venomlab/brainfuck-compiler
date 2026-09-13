#pragma once

#include <iosfwd>
#include <llvm/TargetParser/Triple.h>

namespace llvm {
class Module;
} // namespace llvm

namespace bfc::llvm {

class AssemblyWriter {
  private:
    ::llvm::Triple target_;

  public:
    explicit AssemblyWriter(::llvm::Triple target);

    void write(::llvm::Module& module, std::ostream& output) const;
};

} // namespace bfc::llvm
