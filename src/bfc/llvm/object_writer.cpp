#include "bfc/llvm/object_writer.hpp"

#include "bfc/llvm/native_emitter.hpp"

#include <llvm/Support/CodeGen.h>
#include <utility>

namespace bfc::llvm {

ObjectWriter::ObjectWriter(::llvm::Triple target) : target_(std::move(target)) {}

const ::llvm::Triple& ObjectWriter::target() const {
    return target_;
}

std::string_view ObjectWriter::file_ext() const {
    return ".o";
}

void ObjectWriter::write(::llvm::Module& module, std::ostream& output) const {
    emit_native(module, target_, output, ::llvm::CodeGenFileType::ObjectFile);
}

} // namespace bfc::llvm
