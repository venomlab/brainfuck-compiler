#pragma once

#include "bfc/core/ast.hpp"
#include "bfc/core/lexer.hpp"

#include <memory>
#include <stdexcept>

namespace bfc::parser {

class ParserException : public std::logic_error {
  public:
    using std::logic_error::logic_error;
};

class Parser {
  private:
    lexer::Tokenizer& tokenizer_;

  public:
    explicit Parser(lexer::Tokenizer& tokenizer);

    [[nodiscard]] std::unique_ptr<ast::Program> parse();
};

} // namespace bfc::parser
