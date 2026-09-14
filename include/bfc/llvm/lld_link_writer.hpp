#pragma once

#include "bfc/llvm/artifact_writer.hpp"
#include "bfc/llvm/object_writer.hpp"

namespace bfc::llvm {

class LLDLinkWriter final : public ArtifactWriter {
  private:
    ObjectWriter object_writer_;

  public:
    explicit LLDLinkWriter(ObjectWriter object_writer);

    [[nodiscard]] std::string_view file_ext() const override;
    void write(::llvm::Module& module, std::ostream& output) const override;
};

} // namespace bfc::llvm
