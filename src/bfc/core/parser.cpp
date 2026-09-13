#include "bfc/core/parser.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace bfc::parser {
namespace {

std::unique_ptr<ast::Node> make_node(const lexer::Token token) {
    switch (token) {
    case lexer::Token::INC:
        return std::make_unique<ast::Inc>();
    case lexer::Token::DEC:
        return std::make_unique<ast::Dec>();
    case lexer::Token::INC_PTR:
        return std::make_unique<ast::MoveRight>();
    case lexer::Token::DEC_PTR:
        return std::make_unique<ast::MoveLeft>();
    case lexer::Token::READ_CHAR:
        return std::make_unique<ast::Read>();
    case lexer::Token::PRINT_CHAR:
        return std::make_unique<ast::Print>();
    case lexer::Token::LOOP_START:
    case lexer::Token::LOOP_END:
        throw ParserException("Unexpected loop token");
    }

    throw ParserException("Unknown token");
}

std::unique_ptr<ast::Loop> parse_loop(lexer::TokenIterator& current, const lexer::TokenSentinel& end) {
    std::vector<std::unique_ptr<ast::Node>> operations;
    current++; // skip this loop's LOOP_START token

    for (; current != end; current++) {
        const lexer::Token token = *current;

        switch (token) {
        case lexer::Token::LOOP_END:
            return std::make_unique<ast::Loop>(std::make_unique<ast::Sequence>(std::move(operations)));
            break;
        case lexer::Token::LOOP_START:
            operations.push_back(parse_loop(current, end));
            break;
        default:
            operations.push_back(make_node(token));
            break;
        }
    }

    throw ParserException("Unclosed loop");
}

std::unique_ptr<ast::Sequence> parse_sequence(lexer::TokenIterator& current, const lexer::TokenSentinel& end) {
    std::vector<std::unique_ptr<ast::Node>> operations;

    for (; current != end; current++) {
        const lexer::Token token = *current;

        switch (token) {
        case lexer::Token::LOOP_END:
            throw ParserException("Unexpected loop end");
            break;
        case lexer::Token::LOOP_START:
            operations.push_back(parse_loop(current, end));
            break;
        default:
            operations.push_back(make_node(token));
            break;
        }
    }

    return std::make_unique<ast::Sequence>(std::move(operations));
}

} // namespace

Parser::Parser(lexer::Tokenizer& tokenizer) : tokenizer_(tokenizer) {}

std::unique_ptr<ast::Program> Parser::parse() {
    auto current = tokenizer_.begin();
    const auto end = tokenizer_.end();

    return std::make_unique<ast::Program>(parse_sequence(current, end));
}

} // namespace bfc::parser
