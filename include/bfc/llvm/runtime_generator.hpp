#pragma once

#include <llvm/TargetParser/Triple.h>

namespace llvm {
class Module;
}

namespace bfc::llvm {

class RuntimeGenerator {
  private:
    ::llvm::Triple target_;

  public:
    explicit RuntimeGenerator(::llvm::Triple target);

    [[nodiscard]] static bool supports(const ::llvm::Triple& target);
    void generate(::llvm::Module& module) const;
};

} // namespace bfc::llvm
