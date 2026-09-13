#pragma once

#include "bfc/core/ast.hpp"

#include <llvm/IR/IRBuilder.h>

namespace bfc::llvm {

class IRGenerator final : public ast::Visitor {
  private:
    ::llvm::Module& module_;
    ::llvm::IRBuilder<> builder_;
    ::llvm::AllocaInst* data_pointer_ = nullptr;

    void visit(const ast::Inc& node) override;
    void visit(const ast::Dec& node) override;
    void visit(const ast::MoveLeft& node) override;
    void visit(const ast::MoveRight& node) override;
    void visit(const ast::Read& node) override;
    void visit(const ast::Print& node) override;
    void visit(const ast::Sequence& node) override;
    void visit(const ast::Loop& node) override;
    void visit(const ast::Program& node) override;

  public:
    explicit IRGenerator(::llvm::Module& module);

    void generate(const ast::Program& program);
};

} // namespace bfc::llvm
