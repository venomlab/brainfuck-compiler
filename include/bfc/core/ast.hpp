#pragma once

#include <memory>
#include <utility>
#include <vector>

namespace bfc::ast {

class Visitor;

class Node {
  protected:
    Node() = default;

  public:
    Node(Node&&) = default;
    Node& operator=(Node&&) = default;

    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    virtual void accept(Visitor& visitor) const = 0;

    virtual ~Node() = default;
};

class Inc : public Node {
  public:
    void accept(Visitor& visitor) const override;
};
class Dec : public Node {
  public:
    void accept(Visitor& visitor) const override;
};
class MoveLeft : public Node {
  public:
    void accept(Visitor& visitor) const override;
};
class MoveRight : public Node {
  public:
    void accept(Visitor& visitor) const override;
};
class Read : public Node {
  public:
    void accept(Visitor& visitor) const override;
};
class Print : public Node {
  public:
    void accept(Visitor& visitor) const override;
};
class Sequence : public Node {
  private:
    std::vector<std::unique_ptr<Node>> operations_;

  public:
    explicit Sequence(std::vector<std::unique_ptr<Node>> operations) : operations_(std::move(operations)) {}

    Sequence(Sequence&&) noexcept = default;
    Sequence& operator=(Sequence&&) noexcept = default;

    [[nodiscard]] const std::vector<std::unique_ptr<Node>>& operations() const noexcept {
        return operations_;
    }

    void accept(Visitor& visitor) const override;

    Sequence() = delete;
};
class Loop : public Node {
  private:
    std::unique_ptr<Sequence> inner_;

  public:
    explicit Loop(std::unique_ptr<Sequence> inner) : inner_(std::move(inner)) {}

    Loop(Loop&&) noexcept = default;
    Loop& operator=(Loop&&) noexcept = default;

    [[nodiscard]] const Sequence& inner() const noexcept {
        return *inner_;
    }

    void accept(Visitor& visitor) const override;

    Loop() = delete;
};
class Program : public Node {
  private:
    std::unique_ptr<Sequence> operations_;

  public:
    explicit Program(std::unique_ptr<Sequence> operations) : operations_(std::move(operations)) {}

    Program(Program&&) noexcept = default;
    Program& operator=(Program&&) noexcept = default;

    [[nodiscard]] const Sequence& operations() const noexcept {
        return *operations_;
    }

    void accept(Visitor& visitor) const override;

    Program() = delete;
};

class Visitor {
  public:
    virtual ~Visitor() = default;

    virtual void visit(const Inc& node) = 0;
    virtual void visit(const Dec& node) = 0;
    virtual void visit(const MoveLeft& node) = 0;
    virtual void visit(const MoveRight& node) = 0;
    virtual void visit(const Read& node) = 0;
    virtual void visit(const Print& node) = 0;
    virtual void visit(const Sequence& node) = 0;
    virtual void visit(const Loop& node) = 0;
    virtual void visit(const Program& node) = 0;
};

} // namespace bfc::ast
