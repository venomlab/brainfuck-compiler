#include "bfc/core/ast.hpp"

namespace bfc::ast {

void Inc::accept(Visitor& visitor) const {
    visitor.visit(*this);
}

void Dec::accept(Visitor& visitor) const {
    visitor.visit(*this);
}

void MoveLeft::accept(Visitor& visitor) const {
    visitor.visit(*this);
}

void MoveRight::accept(Visitor& visitor) const {
    visitor.visit(*this);
}

void Read::accept(Visitor& visitor) const {
    visitor.visit(*this);
}

void Print::accept(Visitor& visitor) const {
    visitor.visit(*this);
}

void Sequence::accept(Visitor& visitor) const {
    visitor.visit(*this);
}

void Loop::accept(Visitor& visitor) const {
    visitor.visit(*this);
}

void Program::accept(Visitor& visitor) const {
    visitor.visit(*this);
}

} // namespace bfc::ast
