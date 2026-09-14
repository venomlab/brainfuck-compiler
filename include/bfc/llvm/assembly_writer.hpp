#pragma once

#include "bfc/llvm/artifact_writer.hpp"

#include <llvm/TargetParser/Triple.h>

namespace bfc::llvm {

class AssemblyWriter final : public ArtifactWriter {
  private:
    ::llvm::Triple target_;

  public:
    explicit AssemblyWriter(::llvm::Triple target);

    [[nodiscard]] std::string_view file_ext() const override;
    void write(::llvm::Module& module, std::ostream& output) const override;
};

} // namespace bfc::llvm
