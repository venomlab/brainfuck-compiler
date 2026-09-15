#pragma once

#include <cstdint>
#include <istream>
#include <iterator>
#include <stdexcept>

namespace bfc::lexer {

enum class Token : std::uint8_t {
    INC = 1,
    DEC = 2,
    INC_PTR = 3,
    DEC_PTR = 4,
    LOOP_START = 5,
    LOOP_END = 6,
    READ_CHAR = 7,
    PRINT_CHAR = 8,
};

class TokenizerException : public std::logic_error {
    using std::logic_error::logic_error;
};

class TokenSentinel {
  public:
    std::istreambuf_iterator<char> end;
    TokenSentinel();
};
class TokenIterator {
    using iterator_concept = std::input_iterator_tag;
    using iterator_category = std::input_iterator_tag;
    using value_type = Token;
    using difference_type = std::ptrdiff_t;

  private:
    std::istreambuf_iterator<char> current;
    std::istreambuf_iterator<char> end;
    void find_next();

  public:
    TokenIterator(std::istream& input);
    TokenIterator& operator++();
    void operator++(int);
    Token operator*() const;
    bool operator==(const TokenSentinel&) const;
};
class Tokenizer {
  private:
    std::istream& input;

  public:
    explicit Tokenizer(std::istream& input_stream) : input(input_stream) {};
    Tokenizer() = delete;

    TokenIterator begin();
    TokenSentinel end();
};
} // namespace bfc::lexer
