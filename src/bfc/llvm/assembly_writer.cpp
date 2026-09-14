#include "bfc/llvm/assembly_writer.hpp"

#include "bfc/llvm/native_emitter.hpp"

#include <llvm/Support/CodeGen.h>
#include <utility>

namespace bfc::llvm {

AssemblyWriter::AssemblyWriter(::llvm::Triple target) : target_(std::move(target)) {}

void AssemblyWriter::write(::llvm::Module& module, std::ostream& output) const {
    emit_native(module, target_, output, ::llvm::CodeGenFileType::AssemblyFile);
}

} // namespace bfc::llvm
