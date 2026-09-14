#pragma once

#include "bfc/llvm/artifact_writer.hpp"

#include <llvm/TargetParser/Triple.h>
#include <memory>

namespace bfc::llvm {

class ExternalLinkWriter final : public ArtifactWriter {
  private:
    ::llvm::Triple target_;
    std::unique_ptr<ArtifactWriter> artifact_writer_;

  public:
    ExternalLinkWriter(::llvm::Triple target, std::unique_ptr<ArtifactWriter> artifact_writer);

    [[nodiscard]] std::string_view file_ext() const override;
    void write(::llvm::Module& module, std::ostream& output) const override;
};

} // namespace bfc::llvm
