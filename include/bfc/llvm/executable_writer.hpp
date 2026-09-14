#pragma once

#include "bfc/llvm/object_writer.hpp"

#include <iosfwd>

namespace llvm {
class Module;
}

namespace bfc::llvm {

class ExecutableWriter {
  private:
    ObjectWriter object_writer_;

  public:
    explicit ExecutableWriter(ObjectWriter object_writer);

    void write(::llvm::Module& module, std::ostream& output) const;
};

} // namespace bfc::llvm
