#include <bfc/core/ast.hpp>
#include <bfc/llvm/ir_generator.hpp>
#include <bfc/llvm/ir_writer.hpp>
#include <cstdint>
#include <gtest/gtest.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace bfc::llvm {
namespace {

template <typename... Nodes>
ast::Program make_program(std::unique_ptr<Nodes>... nodes) {
    std::vector<std::unique_ptr<ast::Node>> operations;
    (operations.push_back(std::move(nodes)), ...);
    return ast::Program(std::make_unique<ast::Sequence>(std::move(operations)));
}

template <typename... Nodes>
std::unique_ptr<ast::Loop> make_loop(std::unique_ptr<Nodes>... nodes) {
    std::vector<std::unique_ptr<ast::Node>> operations;
    (operations.push_back(std::move(nodes)), ...);
    return std::make_unique<ast::Loop>(std::make_unique<ast::Sequence>(std::move(operations)));
}

::testing::AssertionResult is_valid(const ::llvm::Module& module) {
    std::string message;
    ::llvm::raw_string_ostream output(message);
    const bool invalid = ::llvm::verifyModule(module, &output);
    output.flush();

    if (invalid) {
        return ::testing::AssertionFailure() << message;
    }
    return ::testing::AssertionSuccess();
}

template <typename Instruction>
std::vector<const Instruction*> find_instructions(const ::llvm::Function& function) {
    std::vector<const Instruction*> result;

    for (const auto& block : function) {
        for (const auto& instruction : block) {
            if (const auto* found = ::llvm::dyn_cast<Instruction>(&instruction)) {
                result.push_back(found);
            }
        }
    }
    return result;
}

const ::llvm::BasicBlock* find_block(const ::llvm::Function& function, const ::llvm::StringRef name) {
    for (const auto& block : function) {
        if (block.getName() == name) {
            return &block;
        }
    }
    return nullptr;
}

TEST(IRGeneratorTest, CreatesTapeAndMainForEmptyProgram) {
    const auto program = make_program();
    ::llvm::LLVMContext context;
    ::llvm::Module module("brainfuck", context);
    IRGenerator generator(module);

    generator.generate(program);

    ASSERT_TRUE(is_valid(module));

    const auto* tape = module.getNamedGlobal("tape");
    ASSERT_NE(tape, nullptr);
    EXPECT_FALSE(tape->isConstant());
    EXPECT_TRUE(tape->hasInternalLinkage());
    EXPECT_TRUE(::llvm::isa<::llvm::ConstantAggregateZero>(tape->getInitializer()));

    const auto* tape_type = ::llvm::dyn_cast<::llvm::ArrayType>(tape->getValueType());
    ASSERT_NE(tape_type, nullptr);
    EXPECT_EQ(tape_type->getNumElements(), std::uint64_t {30'000});
    EXPECT_TRUE(tape_type->getElementType()->isIntegerTy(8));

    const auto* main = module.getFunction("main");
    ASSERT_NE(main, nullptr);
    EXPECT_TRUE(main->hasExternalLinkage());
    EXPECT_TRUE(main->getReturnType()->isIntegerTy(32));
    EXPECT_TRUE(main->arg_empty());
    ASSERT_EQ(main->size(), 1U);

    const auto allocas = find_instructions<::llvm::AllocaInst>(*main);
    ASSERT_EQ(allocas.size(), 1U);
    EXPECT_TRUE(allocas.front()->getAllocatedType()->isPointerTy());

    const auto* return_instruction = ::llvm::dyn_cast<::llvm::ReturnInst>(main->back().getTerminator());
    ASSERT_NE(return_instruction, nullptr);
    const auto* status = ::llvm::dyn_cast<::llvm::ConstantInt>(return_instruction->getReturnValue());
    ASSERT_NE(status, nullptr);
    EXPECT_TRUE(status->isZero());
}

TEST(IRGeneratorTest, GeneratesWrappingCellArithmetic) {
    const auto program = make_program(std::make_unique<ast::Inc>(), std::make_unique<ast::Dec>());
    ::llvm::LLVMContext context;
    ::llvm::Module module("brainfuck", context);
    IRGenerator generator(module);

    generator.generate(program);

    ASSERT_TRUE(is_valid(module));
    const auto* main = module.getFunction("main");
    ASSERT_NE(main, nullptr);

    const auto operations = find_instructions<::llvm::BinaryOperator>(*main);
    ASSERT_EQ(operations.size(), 2U);
    EXPECT_EQ(operations[0]->getOpcode(), ::llvm::Instruction::Add);
    EXPECT_EQ(operations[1]->getOpcode(), ::llvm::Instruction::Sub);

    for (const auto* operation : operations) {
        EXPECT_TRUE(operation->getType()->isIntegerTy(8));
        EXPECT_FALSE(operation->hasNoUnsignedWrap());
        EXPECT_FALSE(operation->hasNoSignedWrap());
    }
}

TEST(IRGeneratorTest, MovesDataPointerBothDirections) {
    const auto program = make_program(std::make_unique<ast::MoveRight>(), std::make_unique<ast::MoveLeft>());
    ::llvm::LLVMContext context;
    ::llvm::Module module("brainfuck", context);
    IRGenerator generator(module);

    generator.generate(program);

    ASSERT_TRUE(is_valid(module));
    const auto* main = module.getFunction("main");
    ASSERT_NE(main, nullptr);

    const auto all_geps = find_instructions<::llvm::GetElementPtrInst>(*main);
    std::vector<const ::llvm::GetElementPtrInst*> movements;
    for (const auto* gep : all_geps) {
        if (gep->getSourceElementType()->isIntegerTy(8)) {
            movements.push_back(gep);
        }
    }

    ASSERT_EQ(movements.size(), 2U);
    EXPECT_FALSE(movements[0]->isInBounds());
    EXPECT_FALSE(movements[1]->isInBounds());

    const auto* right_offset = ::llvm::dyn_cast<::llvm::ConstantInt>(movements[0]->getOperand(1));
    const auto* left_offset = ::llvm::dyn_cast<::llvm::ConstantInt>(movements[1]->getOperand(1));
    ASSERT_NE(right_offset, nullptr);
    ASSERT_NE(left_offset, nullptr);
    EXPECT_TRUE(right_offset->isOne());
    EXPECT_TRUE(left_offset->isMinusOne());
}

TEST(IRGeneratorTest, GeneratesCharacterIOAndMapsEOFToZero) {
    const auto program = make_program(std::make_unique<ast::Read>(), std::make_unique<ast::Print>());
    ::llvm::LLVMContext context;
    ::llvm::Module module("brainfuck", context);
    IRGenerator generator(module);

    generator.generate(program);

    ASSERT_TRUE(is_valid(module));

    const auto* getchar = module.getFunction("getchar");
    ASSERT_NE(getchar, nullptr);
    EXPECT_TRUE(getchar->isDeclaration());
    EXPECT_TRUE(getchar->getReturnType()->isIntegerTy(32));
    EXPECT_TRUE(getchar->arg_empty());

    const auto* putchar = module.getFunction("putchar");
    ASSERT_NE(putchar, nullptr);
    EXPECT_TRUE(putchar->isDeclaration());
    EXPECT_TRUE(putchar->getReturnType()->isIntegerTy(32));
    ASSERT_EQ(putchar->arg_size(), 1U);
    EXPECT_TRUE(putchar->getFunctionType()->getParamType(0)->isIntegerTy(32));

    const auto* main = module.getFunction("main");
    ASSERT_NE(main, nullptr);

    const auto calls = find_instructions<::llvm::CallInst>(*main);
    ASSERT_EQ(calls.size(), 2U);
    ASSERT_NE(calls[0]->getCalledFunction(), nullptr);
    ASSERT_NE(calls[1]->getCalledFunction(), nullptr);
    EXPECT_EQ(calls[0]->getCalledFunction()->getName(), "getchar");
    EXPECT_EQ(calls[1]->getCalledFunction()->getName(), "putchar");

    const auto comparisons = find_instructions<::llvm::ICmpInst>(*main);
    ASSERT_EQ(comparisons.size(), 1U);
    EXPECT_EQ(comparisons.front()->getPredicate(), ::llvm::CmpInst::ICMP_EQ);
    const auto* eof = ::llvm::dyn_cast<::llvm::ConstantInt>(comparisons.front()->getOperand(1));
    ASSERT_NE(eof, nullptr);
    EXPECT_TRUE(eof->isMinusOne());

    const auto selects = find_instructions<::llvm::SelectInst>(*main);
    ASSERT_EQ(selects.size(), 1U);
    const auto* eof_value = ::llvm::dyn_cast<::llvm::ConstantInt>(selects.front()->getTrueValue());
    ASSERT_NE(eof_value, nullptr);
    EXPECT_TRUE(eof_value->isZero());
    EXPECT_TRUE(::llvm::isa<::llvm::TruncInst>(selects.front()->getFalseValue()));

    const auto extensions = find_instructions<::llvm::ZExtInst>(*main);
    ASSERT_EQ(extensions.size(), 1U);
    EXPECT_TRUE(extensions.front()->getSrcTy()->isIntegerTy(8));
    EXPECT_TRUE(extensions.front()->getDestTy()->isIntegerTy(32));
}

TEST(IRGeneratorTest, GeneratesLoopControlFlow) {
    const auto program = make_program(make_loop(std::make_unique<ast::Dec>()));
    ::llvm::LLVMContext context;
    ::llvm::Module module("brainfuck", context);
    IRGenerator generator(module);

    generator.generate(program);

    ASSERT_TRUE(is_valid(module));
    const auto* main = module.getFunction("main");
    ASSERT_NE(main, nullptr);
    ASSERT_EQ(main->size(), 4U);

    const auto* entry = find_block(*main, "entry");
    const auto* condition = find_block(*main, "loop.condition");
    const auto* body = find_block(*main, "loop.body");
    const auto* end = find_block(*main, "loop.end");
    ASSERT_NE(entry, nullptr);
    ASSERT_NE(condition, nullptr);
    ASSERT_NE(body, nullptr);
    ASSERT_NE(end, nullptr);

    const auto* entry_branch = ::llvm::dyn_cast<::llvm::BranchInst>(entry->getTerminator());
    ASSERT_NE(entry_branch, nullptr);
    EXPECT_TRUE(entry_branch->isUnconditional());
    EXPECT_EQ(entry_branch->getSuccessor(0), condition);

    const auto* condition_branch = ::llvm::dyn_cast<::llvm::BranchInst>(condition->getTerminator());
    ASSERT_NE(condition_branch, nullptr);
    EXPECT_TRUE(condition_branch->isConditional());
    EXPECT_EQ(condition_branch->getSuccessor(0), body);
    EXPECT_EQ(condition_branch->getSuccessor(1), end);

    const auto* body_branch = ::llvm::dyn_cast<::llvm::BranchInst>(body->getTerminator());
    ASSERT_NE(body_branch, nullptr);
    EXPECT_TRUE(body_branch->isUnconditional());
    EXPECT_EQ(body_branch->getSuccessor(0), condition);
    EXPECT_TRUE(::llvm::isa<::llvm::ReturnInst>(end->getTerminator()));
}

TEST(IRGeneratorTest, GeneratesNestedLoops) {
    const auto program =
        make_program(make_loop(make_loop(std::make_unique<ast::Inc>()), std::make_unique<ast::MoveRight>()));
    ::llvm::LLVMContext context;
    ::llvm::Module module("brainfuck", context);
    IRGenerator generator(module);

    generator.generate(program);

    ASSERT_TRUE(is_valid(module));
    const auto* main = module.getFunction("main");
    ASSERT_NE(main, nullptr);
    EXPECT_EQ(main->size(), 7U);

    std::size_t conditional_branches = 0;
    for (const auto& block : *main) {
        const auto* branch = ::llvm::dyn_cast<::llvm::BranchInst>(block.getTerminator());
        if (branch != nullptr && branch->isConditional()) {
            conditional_branches++;
        }
    }
    EXPECT_EQ(conditional_branches, 2U);
}

TEST(IRGeneratorTest, WritesGeneratedModule) {
    const auto program = make_program(std::make_unique<ast::Inc>(), std::make_unique<ast::Print>());
    ::llvm::LLVMContext context;
    ::llvm::Module module("brainfuck", context);
    IRGenerator generator(module);
    generator.generate(program);
    ASSERT_TRUE(is_valid(module));

    std::ostringstream output;
    const IRWriter writer;
    writer.write(module, output);

    EXPECT_NE(output.str().find("@tape"), std::string::npos);
    EXPECT_NE(output.str().find("define i32 @main()"), std::string::npos);
    EXPECT_NE(output.str().find("call i32 @putchar"), std::string::npos);
}

} // namespace
} // namespace bfc::llvm
