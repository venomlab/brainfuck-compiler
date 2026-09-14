#pragma once

#include "bfc/llvm/artifact_writer.hpp"

#include <llvm/TargetParser/Triple.h>

namespace bfc::llvm {

class ObjectWriter final : public ArtifactWriter {
  private:
    ::llvm::Triple target_;

  public:
    explicit ObjectWriter(::llvm::Triple target);

    [[nodiscard]] const ::llvm::Triple& target() const;
    void write(::llvm::Module& module, std::ostream& output) const override;
};

} // namespace bfc::llvm
