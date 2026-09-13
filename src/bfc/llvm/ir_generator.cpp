#include "bfc/llvm/ir_generator.hpp"

#include <cstdint>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>

namespace bfc::llvm {
namespace {

constexpr std::uint64_t tape_size = 30'000;

::llvm::Value* load_data_pointer(::llvm::IRBuilder<>& builder, ::llvm::AllocaInst* data_pointer) {
    return builder.CreateLoad(builder.getPtrTy(), data_pointer, "data.pointer");
}

::llvm::Value* load_cell(::llvm::IRBuilder<>& builder, ::llvm::AllocaInst* data_pointer) {
    return builder.CreateLoad(builder.getInt8Ty(), load_data_pointer(builder, data_pointer), "cell");
}

void store_cell(::llvm::IRBuilder<>& builder, ::llvm::AllocaInst* data_pointer, ::llvm::Value* value) {
    builder.CreateStore(value, load_data_pointer(builder, data_pointer));
}

} // namespace

IRGenerator::IRGenerator(::llvm::Module& module) : module_(module), builder_(module.getContext()) {}

void IRGenerator::generate(const ast::Program& program) {
    // Yeah, it's just "scary implementation details"
    // behind "beautiful public interface"
    // You've noticed correctly :rofl:
    program.accept(*this);
}

void IRGenerator::visit(const ast::Inc&) {
    auto* cell = load_cell(builder_, data_pointer_);
    auto* incremented = builder_.CreateAdd(cell, builder_.getInt8(1), "incremented");
    store_cell(builder_, data_pointer_, incremented);
}

void IRGenerator::visit(const ast::Dec&) {
    auto* cell = load_cell(builder_, data_pointer_);
    auto* decremented = builder_.CreateSub(cell, builder_.getInt8(1), "decremented");
    store_cell(builder_, data_pointer_, decremented);
}

void IRGenerator::visit(const ast::MoveLeft&) {
    auto* data_pointer = load_data_pointer(builder_, data_pointer_);
    auto* offset = ::llvm::ConstantInt::getSigned(builder_.getInt64Ty(), -1);
    auto* previous_cell = builder_.CreateGEP(builder_.getInt8Ty(), data_pointer, offset, "previous.cell");
    builder_.CreateStore(previous_cell, data_pointer);
}

void IRGenerator::visit(const ast::MoveRight&) {
    auto* data_pointer = load_data_pointer(builder_, data_pointer_);
    auto* next_cell = builder_.CreateGEP(builder_.getInt8Ty(), data_pointer, builder_.getInt64(1), "next.cell");
    builder_.CreateStore(next_cell, data_pointer);
}

void IRGenerator::visit(const ast::Read&) {
    auto* function_type = ::llvm::FunctionType::get(builder_.getInt32Ty(), false);
    auto getchar = module_.getOrInsertFunction("getchar", function_type);
    auto* input = builder_.CreateCall(getchar, {}, "input");
    auto* eof = ::llvm::ConstantInt::getSigned(builder_.getInt32Ty(), -1);
    auto* is_eof = builder_.CreateICmpEQ(input, eof, "is.eof");
    auto* character = builder_.CreateTrunc(input, builder_.getInt8Ty(), "character");
    auto* value = builder_.CreateSelect(is_eof, builder_.getInt8(0), character, "input.value");
    store_cell(builder_, data_pointer_, value);
}

void IRGenerator::visit(const ast::Print&) {
    auto* function_type = ::llvm::FunctionType::get(builder_.getInt32Ty(), {builder_.getInt32Ty()}, false);
    auto putchar = module_.getOrInsertFunction("putchar", function_type);
    auto* character = builder_.CreateZExt(load_cell(builder_, data_pointer_), builder_.getInt32Ty(), "character");
    builder_.CreateCall(putchar, {character});
}

void IRGenerator::visit(const ast::Sequence& node) {
    for (const auto& operation : node.operations()) {
        operation->accept(*this);
    }
}

void IRGenerator::visit(const ast::Loop& node) {
    auto* function = builder_.GetInsertBlock()->getParent();
    auto* condition = ::llvm::BasicBlock::Create(module_.getContext(), "loop.condition", function);
    auto* body = ::llvm::BasicBlock::Create(module_.getContext(), "loop.body", function);
    auto* end = ::llvm::BasicBlock::Create(module_.getContext(), "loop.end", function);

    builder_.CreateBr(condition);

    builder_.SetInsertPoint(condition);
    auto* cell = load_cell(builder_, data_pointer_);
    auto* should_continue = builder_.CreateICmpNE(cell, builder_.getInt8(0), "loop.should_continue");
    builder_.CreateCondBr(should_continue, body, end);

    builder_.SetInsertPoint(body);
    node.inner().accept(*this);
    builder_.CreateBr(condition);

    builder_.SetInsertPoint(end);
}

void IRGenerator::visit(const ast::Program& node) {
    auto* tape_type = ::llvm::ArrayType::get(builder_.getInt8Ty(), tape_size);
    auto* tape = new ::llvm::GlobalVariable(module_, tape_type, false, ::llvm::GlobalValue::InternalLinkage,
                                            ::llvm::ConstantAggregateZero::get(tape_type), "tape");

    auto* main_type = ::llvm::FunctionType::get(builder_.getInt32Ty(), false);
    auto* main = ::llvm::Function::Create(main_type, ::llvm::GlobalValue::ExternalLinkage, "main", module_);
    auto* entry = ::llvm::BasicBlock::Create(module_.getContext(), "entry", main);
    builder_.SetInsertPoint(entry);

    data_pointer_ = builder_.CreateAlloca(builder_.getPtrTy(), nullptr, "data.pointer");
    auto* tape_begin =
        builder_.CreateInBoundsGEP(tape_type, tape, {builder_.getInt64(0), builder_.getInt64(0)}, "tape.begin");
    builder_.CreateStore(tape_begin, data_pointer_);

    node.operations().accept(*this);
    builder_.CreateRet(builder_.getInt32(0));
}

} // namespace bfc::llvm
