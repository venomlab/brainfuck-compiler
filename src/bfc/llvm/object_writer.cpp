#include "bfc/llvm/object_writer.hpp"

#include "native_emitter.hpp"

#include <llvm/Support/CodeGen.h>
#include <utility>

namespace bfc::llvm {

ObjectWriter::ObjectWriter(::llvm::Triple target) : target_(std::move(target)) {}

void ObjectWriter::write(::llvm::Module& module, std::ostream& output) const {
    emit_native(module, target_, output, ::llvm::CodeGenFileType::ObjectFile);
}

} // namespace bfc::llvm
