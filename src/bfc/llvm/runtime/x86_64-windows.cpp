#include "bfc/llvm/runtime/x86_64-windows.hpp"

#include <cstdint>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/TargetParser/Triple.h>
#include <stdexcept>

namespace bfc::llvm::runtime::x86_64_windows {
namespace {

constexpr std::int64_t standard_input_handle = -10;
constexpr std::int64_t standard_output_handle = -11;

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

::llvm::Function* get_std_handle(::llvm::Module& module, ::llvm::IRBuilder<>& builder) {
    auto* type = ::llvm::FunctionType::get(builder.getPtrTy(), {builder.getInt32Ty()}, false);
    return get_or_create_function(module, "GetStdHandle", type);
}

::llvm::Function* get_read_file(::llvm::Module& module, ::llvm::IRBuilder<>& builder) {
    auto* type = ::llvm::FunctionType::get(
        builder.getInt32Ty(),
        {builder.getPtrTy(), builder.getPtrTy(), builder.getInt32Ty(), builder.getPtrTy(), builder.getPtrTy()}, false);
    return get_or_create_function(module, "ReadFile", type);
}

::llvm::Function* get_write_file(::llvm::Module& module, ::llvm::IRBuilder<>& builder) {
    auto* type = ::llvm::FunctionType::get(
        builder.getInt32Ty(),
        {builder.getPtrTy(), builder.getPtrTy(), builder.getInt32Ty(), builder.getPtrTy(), builder.getPtrTy()}, false);
    return get_or_create_function(module, "WriteFile", type);
}

::llvm::Function* get_exit_process(::llvm::Module& module, ::llvm::IRBuilder<>& builder) {
    auto* type = ::llvm::FunctionType::get(builder.getVoidTy(), {builder.getInt32Ty()}, false);
    auto* function = get_or_create_function(module, "ExitProcess", type);
    function->addFnAttr(::llvm::Attribute::NoReturn);
    return function;
}

::llvm::Value* create_standard_handle(::llvm::Module& module, ::llvm::IRBuilder<>& builder, const std::int64_t handle) {
    auto* handle_id = ::llvm::ConstantInt::getSigned(builder.getInt32Ty(), handle);
    return builder.CreateCall(get_std_handle(module, builder), {handle_id}, "standard.handle");
}

::llvm::Value* create_io_succeeded(::llvm::IRBuilder<>& builder, ::llvm::Value* call_result,
                                   ::llvm::AllocaInst* byte_count) {
    auto* call_succeeded = builder.CreateICmpNE(call_result, builder.getInt32(0), "call.succeeded");
    auto* transferred = builder.CreateLoad(builder.getInt32Ty(), byte_count, "bytes.transferred");
    auto* transferred_one = builder.CreateICmpEQ(transferred, builder.getInt32(1), "transferred.one");
    return builder.CreateAnd(call_succeeded, transferred_one, "io.succeeded");
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
    auto* bytes_read = builder.CreateAlloca(builder.getInt32Ty(), nullptr, "bytes.read");
    builder.CreateStore(builder.getInt32(0), bytes_read);
    auto* handle = create_standard_handle(module, builder, standard_input_handle);
    auto* read_result = builder.CreateCall(
        get_read_file(module, builder),
        {handle, character, builder.getInt32(1), bytes_read, ::llvm::ConstantPointerNull::get(builder.getPtrTy())},
        "read.result");
    builder.CreateCondBr(create_io_succeeded(builder, read_result, bytes_read), success, failure);

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
    auto* bytes_written = builder.CreateAlloca(builder.getInt32Ty(), nullptr, "bytes.written");
    builder.CreateStore(builder.getInt32(0), bytes_written);
    auto* handle = create_standard_handle(module, builder, standard_output_handle);
    auto* write_result = builder.CreateCall(
        get_write_file(module, builder),
        {handle, buffer, builder.getInt32(1), bytes_written, ::llvm::ConstantPointerNull::get(builder.getPtrTy())},
        "write.result");
    auto* succeeded = create_io_succeeded(builder, write_result, bytes_written);
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
    auto* start = get_or_create_function(module, "mainCRTStartup", start_type);
    if (!start->empty()) {
        return;
    }
    start->addFnAttr(::llvm::Attribute::NoReturn);

    auto* entry = ::llvm::BasicBlock::Create(module.getContext(), "entry", start);
    builder.SetInsertPoint(entry);

    auto* exit_code = builder.CreateCall(main, {}, "exit.code");
    builder.CreateCall(get_exit_process(module, builder), {exit_code});
    builder.CreateUnreachable();
}

void generate_mingw_startup(::llvm::Module& module) {
    ::llvm::IRBuilder<> builder(module.getContext());
    auto* function_type = ::llvm::FunctionType::get(builder.getVoidTy(), false);
    auto* function = get_or_create_function(module, "__main", function_type);
    if (!function->empty()) {
        return;
    }

    auto* entry = ::llvm::BasicBlock::Create(module.getContext(), "entry", function);
    builder.SetInsertPoint(entry);
    builder.CreateRetVoid();
}

} // namespace

bool supports(const ::llvm::Triple& target) {
    return target.getArch() == ::llvm::Triple::x86_64 && target.isOSWindows();
}

void generate(::llvm::Module& module) {
    generate_getchar(module);
    generate_putchar(module);
    generate_start(module);
    if (::llvm::Triple(module.getTargetTriple()).isWindowsGNUEnvironment()) {
        generate_mingw_startup(module);
    }
}

} // namespace bfc::llvm::runtime::x86_64_windows
