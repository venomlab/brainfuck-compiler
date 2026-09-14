#pragma once

#include <stdexcept>

namespace bfc::llvm {

class ExecutableEmissionException : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};

} // namespace bfc::llvm
