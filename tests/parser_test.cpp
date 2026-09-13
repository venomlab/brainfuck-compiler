#include <bfc/core/parser.hpp>
#include <gtest/gtest.h>
#include <sstream>
#include <string>

namespace bfc::parser {
namespace {

std::unique_ptr<ast::Program> parse_source(const std::string& source) {
    std::istringstream input {source};
    lexer::Tokenizer tokenizer {input};
    Parser parser {tokenizer};

    return parser.parse();
}

TEST(ParserTest, ParsesEmptyProgram) {
    const auto program = parse_source("");

    EXPECT_TRUE(program->operations().operations().empty());
}

TEST(ParserTest, ParsesSimpleOperations) {
    const auto program = parse_source("+-><,.");
    const auto& operations = program->operations().operations();

    ASSERT_EQ(operations.size(), 6U);
    EXPECT_NE(dynamic_cast<const ast::Inc*>(operations[0].get()), nullptr);
    EXPECT_NE(dynamic_cast<const ast::Dec*>(operations[1].get()), nullptr);
    EXPECT_NE(dynamic_cast<const ast::MoveRight*>(operations[2].get()), nullptr);
    EXPECT_NE(dynamic_cast<const ast::MoveLeft*>(operations[3].get()), nullptr);
    EXPECT_NE(dynamic_cast<const ast::Read*>(operations[4].get()), nullptr);
    EXPECT_NE(dynamic_cast<const ast::Print*>(operations[5].get()), nullptr);
}

TEST(ParserTest, ParsesSingleLoop) {
    const auto program = parse_source("[+-]");
    const auto& operations = program->operations().operations();

    ASSERT_EQ(operations.size(), 1U);
    const auto* loop = dynamic_cast<const ast::Loop*>(operations[0].get());
    ASSERT_NE(loop, nullptr);

    const auto& inner_operations = loop->inner().operations();
    ASSERT_EQ(inner_operations.size(), 2U);
    EXPECT_NE(dynamic_cast<const ast::Inc*>(inner_operations[0].get()), nullptr);
    EXPECT_NE(dynamic_cast<const ast::Dec*>(inner_operations[1].get()), nullptr);
}

TEST(ParserTest, ParsesNestedLoops) {
    const auto program = parse_source("[[]]");
    const auto& operations = program->operations().operations();

    ASSERT_EQ(operations.size(), 1U);
    const auto* outer_loop = dynamic_cast<const ast::Loop*>(operations[0].get());
    ASSERT_NE(outer_loop, nullptr);

    const auto& outer_operations = outer_loop->inner().operations();
    ASSERT_EQ(outer_operations.size(), 1U);
    const auto* inner_loop = dynamic_cast<const ast::Loop*>(outer_operations[0].get());
    ASSERT_NE(inner_loop, nullptr);
    EXPECT_TRUE(inner_loop->inner().operations().empty());
}

TEST(ParserTest, ParsesOperationsAroundLoop) {
    const auto program = parse_source("+[-]>");
    const auto& operations = program->operations().operations();

    ASSERT_EQ(operations.size(), 3U);
    EXPECT_NE(dynamic_cast<const ast::Inc*>(operations[0].get()), nullptr);

    const auto* loop = dynamic_cast<const ast::Loop*>(operations[1].get());
    ASSERT_NE(loop, nullptr);
    const auto& inner_operations = loop->inner().operations();
    ASSERT_EQ(inner_operations.size(), 1U);
    EXPECT_NE(dynamic_cast<const ast::Dec*>(inner_operations[0].get()), nullptr);

    EXPECT_NE(dynamic_cast<const ast::MoveRight*>(operations[2].get()), nullptr);
}

TEST(ParserTest, IgnoresNonBrainfuckCharacters) {
    const auto program = parse_source("before + words [ ignored - ] after .");
    const auto& operations = program->operations().operations();

    ASSERT_EQ(operations.size(), 3U);
    EXPECT_NE(dynamic_cast<const ast::Inc*>(operations[0].get()), nullptr);

    const auto* loop = dynamic_cast<const ast::Loop*>(operations[1].get());
    ASSERT_NE(loop, nullptr);
    const auto& inner_operations = loop->inner().operations();
    ASSERT_EQ(inner_operations.size(), 1U);
    EXPECT_NE(dynamic_cast<const ast::Dec*>(inner_operations[0].get()), nullptr);

    EXPECT_NE(dynamic_cast<const ast::Print*>(operations[2].get()), nullptr);
}

TEST(ParserTest, RejectsUnexpectedLoopEnd) {
    EXPECT_THROW((void)parse_source("]"), ParserException);
}

TEST(ParserTest, RejectsUnclosedLoop) {
    EXPECT_THROW((void)parse_source("[+"), ParserException);
}

} // namespace
} // namespace bfc::parser
