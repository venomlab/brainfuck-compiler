#include "bfc/llvm/ir_writer.hpp"

#include <llvm/IR/Module.h>
#include <llvm/Support/raw_os_ostream.h>

namespace bfc::llvm {

void IRWriter::write(::llvm::Module& module, std::ostream& output) const {
    ::llvm::raw_os_ostream llvm_output {output};
    module.print(llvm_output, nullptr);
    llvm_output.flush();
}

} // namespace bfc::llvm
