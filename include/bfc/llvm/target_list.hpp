#pragma once

#include <iosfwd>

namespace bfc::llvm {

void write_target_list(std::ostream& output, std::ostream& error);

} // namespace bfc::llvm
