#pragma once

#include "bfc/llvm/artifact_writer.hpp"

namespace bfc::llvm {

class IRWriter final : public ArtifactWriter {
  public:
    [[nodiscard]] std::string_view file_ext() const override;
    void write(::llvm::Module& module, std::ostream& output) const override;
};

} // namespace bfc::llvm
