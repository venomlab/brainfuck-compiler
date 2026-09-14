#include "bfc/llvm/runtime/x86_64-linux.hpp"

#include <cstdint>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Module.h>
#include <llvm/TargetParser/Triple.h>
#include <stdexcept>

namespace bfc::llvm::runtime::x86_64_linux {
namespace {

constexpr std::uint64_t read_syscall = 0;
constexpr std::uint64_t write_syscall = 1;
constexpr std::uint64_t exit_syscall = 60;

::llvm::Function* get_or_create_function(::llvm::Module& module, const ::llvm::StringRef name,
                                         ::llvm::FunctionType* type) {
    if (auto* function = module.getFunction(name)) {
        if (function->getFunctionType() != type) {
            throw std::runtime_error("Runtime function has incompatible type: " + name.str());
        }
        return function;
    }

    return ::llvm::Function::Create(type, ::llvm::GlobalValue::ExternalLinkage, name, module);
}

::llvm::CallInst* create_syscall(::llvm::IRBuilder<>& builder, const std::uint64_t number,
                                 ::llvm::Value* first_argument, ::llvm::Value* second_argument,
                                 ::llvm::Value* third_argument) {
    auto* syscall_type = ::llvm::FunctionType::get(
        builder.getInt64Ty(), {builder.getInt64Ty(), builder.getPtrTy(), builder.getInt64Ty(), builder.getInt64Ty()},
        false);
    auto* syscall =
        ::llvm::InlineAsm::get(syscall_type, "syscall",
                               "={rax},{di},{si},{dx},{rax},~{rcx},~{r11},~{memory},~{dirflag},~{fpsr},~{flags}", true);

    return builder.CreateCall(syscall, {first_argument, second_argument, third_argument, builder.getInt64(number)},
                              "syscall.result");
}

void generate_getchar(::llvm::Module& module) {
    ::llvm::IRBuilder<> builder(module.getContext());
    auto* function_type = ::llvm::FunctionType::get(builder.getInt32Ty(), false);
    auto* function = get_or_create_function(module, "getchar", function_type);
    if (!function->empty()) {
        return;
    }

    auto* entry = ::llvm::BasicBlock::Create(module.getContext(), "entry", function);
    auto* success = ::llvm::BasicBlock::Create(module.getContext(), "success", function);
    auto* failure = ::llvm::BasicBlock::Create(module.getContext(), "failure", function);
    auto* end = ::llvm::BasicBlock::Create(module.getContext(), "end", function);
    builder.SetInsertPoint(entry);

    auto* character = builder.CreateAlloca(builder.getInt8Ty(), nullptr, "character");
    auto* bytes_read = create_syscall(builder, read_syscall, builder.getInt64(0), character, builder.getInt64(1));
    auto* succeeded = builder.CreateICmpEQ(bytes_read, builder.getInt64(1), "read.succeeded");
    builder.CreateCondBr(succeeded, success, failure);

    builder.SetInsertPoint(success);
    auto* input = builder.CreateLoad(builder.getInt8Ty(), character, "input");
    auto* input_value = builder.CreateZExt(input, builder.getInt32Ty(), "input.value");
    builder.CreateBr(end);

    builder.SetInsertPoint(failure);
    builder.CreateBr(end);

    builder.SetInsertPoint(end);
    auto* result = builder.CreatePHI(builder.getInt32Ty(), 2, "result");
    result->addIncoming(input_value, success);
    result->addIncoming(::llvm::ConstantInt::getSigned(builder.getInt32Ty(), -1), failure);
    builder.CreateRet(result);
}

void generate_putchar(::llvm::Module& module) {
    ::llvm::IRBuilder<> builder(module.getContext());
    auto* function_type = ::llvm::FunctionType::get(builder.getInt32Ty(), {builder.getInt32Ty()}, false);
    auto* function = get_or_create_function(module, "putchar", function_type);
    if (!function->empty()) {
        return;
    }

    auto* character = function->getArg(0);
    character->setName("character");

    auto* entry = ::llvm::BasicBlock::Create(module.getContext(), "entry", function);
    builder.SetInsertPoint(entry);

    auto* byte = builder.CreateTrunc(character, builder.getInt8Ty(), "byte");
    auto* buffer = builder.CreateAlloca(builder.getInt8Ty(), nullptr, "buffer");
    builder.CreateStore(byte, buffer);
    auto* bytes_written = create_syscall(builder, write_syscall, builder.getInt64(1), buffer, builder.getInt64(1));
    auto* succeeded = builder.CreateICmpEQ(bytes_written, builder.getInt64(1), "write.succeeded");
    auto* written_character = builder.CreateZExt(byte, builder.getInt32Ty(), "written.character");
    auto* result = builder.CreateSelect(succeeded, written_character,
                                        ::llvm::ConstantInt::getSigned(builder.getInt32Ty(), -1), "result");
    builder.CreateRet(result);
}

void generate_start(::llvm::Module& module) {
    ::llvm::IRBuilder<> builder(module.getContext());
    auto* main_type = ::llvm::FunctionType::get(builder.getInt32Ty(), false);
    auto* main = module.getFunction("main");
    if (main == nullptr || main->getFunctionType() != main_type || main->isDeclaration()) {
        throw std::runtime_error("Runtime requires a defined main function with type i32 ()");
    }

    auto* start_type = ::llvm::FunctionType::get(builder.getVoidTy(), false);
    auto* start = get_or_create_function(module, "_start", start_type);
    if (!start->empty()) {
        return;
    }
    start->addFnAttr(::llvm::Attribute::NoReturn);
    start->addFnAttr("stackrealign");

    auto* entry = ::llvm::BasicBlock::Create(module.getContext(), "entry", start);
    builder.SetInsertPoint(entry);

    auto* exit_code = builder.CreateSExt(builder.CreateCall(main), builder.getInt64Ty(), "exit.code");
    create_syscall(builder, exit_syscall, exit_code, ::llvm::ConstantPointerNull::get(builder.getPtrTy()),
                   builder.getInt64(0));
    builder.CreateUnreachable();
}

} // namespace

bool supports(const ::llvm::Triple& target) {
    return target.getArch() == ::llvm::Triple::x86_64 && target.isOSLinux() &&
           target.getEnvironment() != ::llvm::Triple::GNUX32;
}

void generate(::llvm::Module& module) {
    generate_getchar(module);
    generate_putchar(module);
    generate_start(module);
}

} // namespace bfc::llvm::runtime::x86_64_linux
