#pragma once

#include "bfc/llvm/object_writer.hpp"

namespace bfc::llvm {

class ExecutableWriter final : public ArtifactWriter {
  private:
    ObjectWriter object_writer_;

  public:
    explicit ExecutableWriter(ObjectWriter object_writer);

    void write(::llvm::Module& module, std::ostream& output) const override;
};

} // namespace bfc::llvm
