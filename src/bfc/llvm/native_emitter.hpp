#pragma once

#include <iosfwd>
#include <llvm/Support/CodeGen.h>

namespace llvm {
class Module;
class Triple;
} // namespace llvm

namespace bfc::llvm {

void emit_native(::llvm::Module& module, const ::llvm::Triple& target, std::ostream& output,
                 ::llvm::CodeGenFileType file_type);

} // namespace bfc::llvm
