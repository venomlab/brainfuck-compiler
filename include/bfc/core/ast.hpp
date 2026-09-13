#pragma once

#include <memory>
#include <utility>
#include <vector>

namespace bfc::ast {

class Node {
  protected:
    Node() = default;

  public:
    Node(Node&&) = default;
    Node& operator=(Node&&) = default;

    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    virtual ~Node() = default;
};

class Inc : public Node {};
class Dec : public Node {};
class MoveLeft : public Node {};
class MoveRight : public Node {};
class Read : public Node {};
class Print : public Node {};
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

    Program() = delete;
};

} // namespace bfc::ast
