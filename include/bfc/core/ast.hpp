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
    std::vector<std::unique_ptr<Node>> operations;

  public:
    explicit Sequence(std::vector<std::unique_ptr<Node>> operations) : operations(std::move(operations)) {}

    Sequence(Sequence&&) noexcept = default;
    Sequence& operator=(Sequence&&) noexcept = default;

    Sequence() = delete;
};
class Loop : public Node {
  private:
    Sequence inner;

  public:
    explicit Loop(Sequence inner) : inner(std::move(inner)) {}

    Loop(Loop&&) noexcept = default;
    Loop& operator=(Loop&&) noexcept = default;

    Loop() = delete;
};
class Program : public Node {
  private:
    Sequence operations;

  public:
    explicit Program(Sequence operations) : operations(std::move(operations)) {}

    Program(Program&&) noexcept = default;
    Program& operator=(Program&&) noexcept = default;

    Program() = delete;
};

} // namespace bfc::ast
