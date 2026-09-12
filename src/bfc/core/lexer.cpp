#include "bfc/core/lexer.hpp"

#include <csignal>
#include <format>
#include <string_view>

namespace bfc::lexer {

constexpr std::string_view parsable_chars = "><+-.,[]";

TokenIterator Tokenizer::begin() {
    return TokenIterator(input);
}
TokenSentinel Tokenizer::end() {
    return TokenSentinel();
}

TokenSentinel::TokenSentinel() : end({}) {}
TokenIterator::TokenIterator(std::istream& input) : current(input), end({}) {
    find_next();
}
void TokenIterator::find_next() {
    for (; current != end; current++) {
        if (parsable_chars.find(*current) != std::string_view::npos)
            break;
    }
}
TokenIterator& TokenIterator::operator++() {
    if (current == end) {
        return *this;
    }
    current++;
    find_next();
    return *this;
}
void TokenIterator::operator++(int) {
    ++(*this);
}
bool TokenIterator::operator==(const TokenSentinel& ts) const {
    return current == ts.end;
}
Token TokenIterator::operator*() const {
    char c = *current;
    switch (c) {
    case '>':
        return Token::INC_PTR;
        break;
    case '<':
        return Token::DEC_PTR;
        break;
    case '+':
        return Token::INC;
        break;
    case '-':
        return Token::DEC;
        break;
    case '.':
        return Token::PRINT_CHAR;
        break;
    case ',':
        return Token::READ_CHAR;
        break;
    case '[':
        return Token::LOOP_START;
        break;
    case ']':
        return Token::LOOP_END;
        break;
    default:
        auto msg = std::format("Unknown symbol slipped into tokenizer: {}", c);
        throw TokenizerException(msg);
        break;
    }
}

} // namespace bfc::lexer
