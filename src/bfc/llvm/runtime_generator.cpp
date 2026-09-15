#include "bfc/llvm/runtime_generator.hpp"

#include "bfc/llvm/runtime/x86_64-linux.hpp"
#include "bfc/llvm/runtime/x86_64-windows.hpp"

#include <llvm/IR/Module.h>
#include <stdexcept>
#include <utility>

namespace bfc::llvm {

RuntimeGenerator::RuntimeGenerator(::llvm::Triple target) : target_(std::move(target)) {}

bool RuntimeGenerator::supports(const ::llvm::Triple& target) {
    return runtime::x86_64_linux::supports(target) || runtime::x86_64_windows::supports(target);
}

void RuntimeGenerator::generate(::llvm::Module& module) const {
    if (runtime::x86_64_linux::supports(target_)) {
        module.setTargetTriple(target_.str());
        runtime::x86_64_linux::generate(module);
        return;
    }
    if (runtime::x86_64_windows::supports(target_)) {
        module.setTargetTriple(target_.str());
        runtime::x86_64_windows::generate(module);
        return;
    }

    throw std::invalid_argument("Unsupported runtime target: " + target_.str());
}

} // namespace bfc::llvm
