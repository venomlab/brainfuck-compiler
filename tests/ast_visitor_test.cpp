#include <bfc/core/ast.hpp>
#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <utility>
#include <vector>

namespace bfc::ast {
namespace {

enum class NodeKind : std::uint8_t {
    Inc,
    Dec,
    MoveLeft,
    MoveRight,
    Read,
    Print,
    Sequence,
    Loop,
    Program,
};

class TraceVisitor final : public Visitor {
  private:
    std::vector<NodeKind> trace_;

  public:
    void visit(const Inc&) override {
        trace_.push_back(NodeKind::Inc);
    }

    void visit(const Dec&) override {
        trace_.push_back(NodeKind::Dec);
    }

    void visit(const MoveLeft&) override {
        trace_.push_back(NodeKind::MoveLeft);
    }

    void visit(const MoveRight&) override {
        trace_.push_back(NodeKind::MoveRight);
    }

    void visit(const Read&) override {
        trace_.push_back(NodeKind::Read);
    }

    void visit(const Print&) override {
        trace_.push_back(NodeKind::Print);
    }

    void visit(const Sequence& node) override {
        trace_.push_back(NodeKind::Sequence);
        for (const auto& operation : node.operations()) {
            operation->accept(*this);
        }
    }

    void visit(const Loop& node) override {
        trace_.push_back(NodeKind::Loop);
        node.inner().accept(*this);
    }

    void visit(const Program& node) override {
        trace_.push_back(NodeKind::Program);
        node.operations().accept(*this);
    }

    [[nodiscard]] const std::vector<NodeKind>& trace() const noexcept {
        return trace_;
    }
};

TEST(AstVisitorTest, DispatchesEveryNodeAndTraversesConstTree) {
    std::vector<std::unique_ptr<Node>> loop_operations;
    loop_operations.push_back(std::make_unique<Dec>());

    std::vector<std::unique_ptr<Node>> program_operations;
    program_operations.push_back(std::make_unique<Inc>());
    program_operations.push_back(std::make_unique<Loop>(std::make_unique<Sequence>(std::move(loop_operations))));
    program_operations.push_back(std::make_unique<MoveLeft>());
    program_operations.push_back(std::make_unique<MoveRight>());
    program_operations.push_back(std::make_unique<Read>());
    program_operations.push_back(std::make_unique<Print>());

    const Program program {std::make_unique<Sequence>(std::move(program_operations))};
    TraceVisitor visitor;

    program.accept(visitor);

    const std::vector<NodeKind> expected {
        NodeKind::Program, NodeKind::Sequence, NodeKind::Inc,       NodeKind::Loop, NodeKind::Sequence,
        NodeKind::Dec,     NodeKind::MoveLeft, NodeKind::MoveRight, NodeKind::Read, NodeKind::Print,
    };
    EXPECT_EQ(visitor.trace(), expected);
}

} // namespace
} // namespace bfc::ast
