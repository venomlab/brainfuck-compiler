#include <bfc/core/lexer.hpp>
#include <gtest/gtest.h>
#include <sstream>
#include <vector>

namespace bfc::lexer {
namespace {

TEST(TokenIteratorTest, ReadsIncrement) {
    std::istringstream input {"+"};
    TokenIterator it {input};

    EXPECT_EQ(*it, Token::INC);
}

TEST(TokenIteratorTest, ReadsDecrement) {
    std::istringstream input {"-"};
    TokenIterator it {input};

    EXPECT_EQ(*it, Token::DEC);
}

TEST(TokenIteratorTest, ReadsPointerIncrement) {
    std::istringstream input {">"};
    TokenIterator it {input};

    EXPECT_EQ(*it, Token::INC_PTR);
}

TEST(TokenIteratorTest, ReadsPointerDecrement) {
    std::istringstream input {"<"};
    TokenIterator it {input};

    EXPECT_EQ(*it, Token::DEC_PTR);
}

TEST(TokenIteratorTest, ReadsLoopStart) {
    std::istringstream input {"["};
    TokenIterator it {input};

    EXPECT_EQ(*it, Token::LOOP_START);
}

TEST(TokenIteratorTest, ReadsLoopEnd) {
    std::istringstream input {"]"};
    TokenIterator it {input};

    EXPECT_EQ(*it, Token::LOOP_END);
}

TEST(TokenIteratorTest, ReadsCharacter) {
    std::istringstream input {","};
    TokenIterator it {input};

    EXPECT_EQ(*it, Token::READ_CHAR);
}

TEST(TokenIteratorTest, PrintsCharacter) {
    std::istringstream input {"."};
    TokenIterator it {input};

    EXPECT_EQ(*it, Token::PRINT_CHAR);
}

TEST(TokenIteratorTest, IteratesOverTokens) {
    std::istringstream input {"+-><[],."};
    TokenIterator it {input};

    EXPECT_EQ(*it, Token::INC);

    ++it;
    EXPECT_EQ(*it, Token::DEC);

    ++it;
    EXPECT_EQ(*it, Token::INC_PTR);

    ++it;
    EXPECT_EQ(*it, Token::DEC_PTR);

    ++it;
    EXPECT_EQ(*it, Token::LOOP_START);

    ++it;
    EXPECT_EQ(*it, Token::LOOP_END);

    ++it;
    EXPECT_EQ(*it, Token::READ_CHAR);

    ++it;
    EXPECT_EQ(*it, Token::PRINT_CHAR);
}

TEST(TokenIteratorTest, PostIncrementAdvancesIterator) {
    std::istringstream input {"+-"};
    TokenIterator it {input};

    EXPECT_EQ(*it, Token::INC);

    it++;

    EXPECT_EQ(*it, Token::DEC);
}

TEST(TokenIteratorTest, IgnoresNonBrainfuckCharacters) {
    std::istringstream input {"hello world + some garbage -"};
    TokenIterator it {input};

    EXPECT_EQ(*it, Token::INC);

    ++it;

    EXPECT_EQ(*it, Token::DEC);
}

TEST(TokenIteratorTest, IgnoresCharactersBeforeFirstToken) {
    std::istringstream input {"this is a comment >"};
    TokenIterator it {input};

    EXPECT_EQ(*it, Token::INC_PTR);
}

TEST(TokenIteratorTest, IsNotEndWhileTokenExists) {
    std::istringstream input {"+"};

    TokenIterator it {input};
    TokenSentinel end {};

    EXPECT_NE(it, end);
}

TEST(TokenIteratorTest, ReachesEndAfterLastToken) {
    std::istringstream input {"++"};

    TokenIterator it {input};
    TokenSentinel end {};

    EXPECT_NE(it, end);

    ++it;
    EXPECT_NE(it, end);

    ++it;
    EXPECT_EQ(it, end);
}

TEST(TokenIteratorTest, EmptyInputIsImmediatelyEnd) {
    std::istringstream input {};

    TokenIterator it {input};
    TokenSentinel end {};

    EXPECT_EQ(it, end);
}

TEST(TokenIteratorTest, InputWithoutTokensIsImmediatelyEnd) {
    std::istringstream input {"hello this is not brainfuck"};

    TokenIterator it {input};
    TokenSentinel end {};

    EXPECT_EQ(it, end);
}

TEST(TokenIteratorTest, ReachesEndAfterTrailingGarbage) {
    std::istringstream input {"+ trailing garbage"};

    TokenIterator it {input};
    TokenSentinel end {};

    ASSERT_NE(it, end);
    EXPECT_EQ(*it, Token::INC);

    ++it;

    EXPECT_EQ(it, end);
}

TEST(TokenIteratorTest, SkipsGarbageBetweenTokens) {
    std::istringstream input {"+ abc xyz >"};

    TokenIterator it {input};
    TokenSentinel end {};

    ASSERT_NE(it, end);
    EXPECT_EQ(*it, Token::INC);

    ++it;

    ASSERT_NE(it, end);
    EXPECT_EQ(*it, Token::INC_PTR);

    ++it;

    EXPECT_EQ(it, end);
}

TEST(TokenIteratorTest, TokenizesCompleteProgram) {
    std::istringstream input {"abc ++[>--<]., xyz"};

    TokenIterator it {input};
    TokenSentinel end {};

    const std::vector<Token> expected {
        Token::INC, Token::INC,     Token::LOOP_START, Token::INC_PTR,    Token::DEC,
        Token::DEC, Token::DEC_PTR, Token::LOOP_END,   Token::PRINT_CHAR, Token::READ_CHAR,
    };

    std::vector<Token> actual;

    while (it != end) {
        actual.push_back(*it);
        ++it;
    }

    EXPECT_EQ(actual, expected);
}

TEST(TokenizerTest, SupportsRangeBasedFor) {
    std::istringstream input {"++[>--<].,"};
    Tokenizer tokenizer {input};

    const std::vector<Token> expected {
        Token::INC, Token::INC,     Token::LOOP_START, Token::INC_PTR,    Token::DEC,
        Token::DEC, Token::DEC_PTR, Token::LOOP_END,   Token::PRINT_CHAR, Token::READ_CHAR,
    };

    std::vector<Token> actual;

    for (const Token token : tokenizer) {
        actual.push_back(token);
    }

    EXPECT_EQ(actual, expected);
}

TEST(TokenizerTest, RangeBasedForSkipsNonBrainfuckCharacters) {
    std::istringstream input {"hello + world [foo > bar -] baz ."};
    Tokenizer tokenizer {input};

    const std::vector<Token> expected {
        Token::INC, Token::LOOP_START, Token::INC_PTR, Token::DEC, Token::LOOP_END, Token::PRINT_CHAR,
    };

    std::vector<Token> actual;

    for (const Token token : tokenizer) {
        actual.push_back(token);
    }

    EXPECT_EQ(actual, expected);
}

TEST(TokenizerTest, EmptyInputProducesNoTokens) {
    std::istringstream input {};
    Tokenizer tokenizer {input};

    std::vector<Token> actual;

    for (const Token token : tokenizer) {
        actual.push_back(token);
    }

    EXPECT_TRUE(actual.empty());
}

TEST(TokenizerTest, InputWithoutBrainfuckCommandsProducesNoTokens) {
    std::istringstream input {"hello world this is just text"};
    Tokenizer tokenizer {input};

    std::vector<Token> actual;

    for (const Token token : tokenizer) {
        actual.push_back(token);
    }

    EXPECT_TRUE(actual.empty());
}

} // namespace
} // namespace bfc::lexer
