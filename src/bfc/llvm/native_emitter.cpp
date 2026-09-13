#include "native_emitter.hpp"

#include "bfc/llvm/native_emission_exception.hpp"

#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Triple.h>
#include <memory>
#include <optional>
#include <ostream>
#include <string>

namespace bfc::llvm {
namespace {

void initialize_native_target() {
    if (::llvm::InitializeNativeTarget()) {
        throw NativeEmissionException("Could not initialize native target");
    }
    if (::llvm::InitializeNativeTargetAsmPrinter()) {
        throw NativeEmissionException("Could not initialize native assembly printer");
    }
}

std::unique_ptr<::llvm::TargetMachine> create_target_machine(const ::llvm::Triple& target) {
    std::string error;
    const auto* target_backend = ::llvm::TargetRegistry::lookupTarget(target.str(), error);
    if (target_backend == nullptr) {
        throw NativeEmissionException("Could not find target " + target.str() + ": " + error);
    }

    ::llvm::TargetOptions options;
    auto target_machine = std::unique_ptr<::llvm::TargetMachine>(target_backend->createTargetMachine(
        target.str(), "generic", "", options, ::llvm::Reloc::PIC_, std::nullopt, ::llvm::CodeGenOptLevel::None));
    if (target_machine == nullptr) {
        throw NativeEmissionException("Could not create target machine for " + target.str());
    }
    return target_machine;
}

} // namespace

void emit_native(::llvm::Module& module, const ::llvm::Triple& target, std::ostream& output,
                 const ::llvm::CodeGenFileType file_type) {
    initialize_native_target();
    auto target_machine = create_target_machine(target);

    module.setTargetTriple(target.str());
    module.setDataLayout(target_machine->createDataLayout());

    ::llvm::SmallVector<char, 0> buffer;
    ::llvm::raw_svector_ostream llvm_output(buffer);
    ::llvm::legacy::PassManager passes;
    if (target_machine->addPassesToEmitFile(passes, llvm_output, nullptr, file_type)) {
        throw NativeEmissionException("Target " + target.str() + " cannot emit requested file type");
    }

    passes.run(module);
    output.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    if (!output) {
        throw NativeEmissionException("Could not write native output");
    }
}

} // namespace bfc::llvm
