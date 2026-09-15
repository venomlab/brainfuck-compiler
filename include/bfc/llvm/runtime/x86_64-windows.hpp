#pragma once

namespace llvm {
class Module;
class Triple;
} // namespace llvm

namespace bfc::llvm::runtime::x86_64_windows {

[[nodiscard]] bool supports(const ::llvm::Triple& target);
void generate(::llvm::Module& module);

} // namespace bfc::llvm::runtime::x86_64_windows
