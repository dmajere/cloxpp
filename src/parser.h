#pragma once

#include <stdint.h>

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "chunk.h"
#include "debug.h"
#include "token.h"

DECLARE_bool(debug);

namespace lox {
namespace lang {
constexpr std::string_view kExpectRightParen = "Expect ')' after expression.";
class Parser;

typedef void (Parser::*ParseFn)();

struct ParseError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

class Parser {
 public:
  Parser(const std::vector<Token>& tokens, Chunk& chunk)
      : tokens_{tokens}, chunk_{chunk}, current_{0} {}

  bool parse() {
    try {
      expression();
      end();
      if (FLAGS_debug && !hadError_) {
        Disassembler::dis(chunk_, "Compiled chunk");
      }
    } catch (ParseError&) {
      synchronize();
    }
    return !hadError_;
  }

 private:
  const std::vector<Token>& tokens_;
  Chunk& chunk_;
  size_t current_;
  bool hadError_{false};

  enum class Precedence {
    NONE,
    ASSIGNMENT,
    OR,
    AND,
    EQUALITY,
    COMPARISON,
    TERM,
    FACTOR,
    UNARY,
    CALL,
    PRIMARY,
  };

  struct ParseRule {
    std::function<void()> prefix;
    std::function<void()> infix;
    const Precedence precedence;
  };

  void emitByte(uint8_t byte) {}

  void emitConstant(double constant) {
    auto offset = chunk_.addConstant(constant);
    chunk_.addCode(OpCode::CONSTANT, previous().line);
    chunk_.addOperand(offset);
  }

  void emitReturn() { chunk_.addCode(OpCode::RETURN, previous().line); }

  void end() { emitReturn(); }

  void parsePrecedence(const Precedence& precedence) {
    advance();
    const auto rule = getRule(previous());
    if (rule.prefix == nullptr) {
      error(previous(), "Expected expression.");
      return;
    }
    rule.prefix();

    while (precedence <= getRule(current()).precedence) {
      advance();
      const auto infixRule = getRule(previous());
      if (infixRule.infix == nullptr) {
        error(previous(), "Expected infix expression.");
        return;
      }
      infixRule.infix();
    }
  }

  void expression() { parsePrecedence(Precedence::ASSIGNMENT); }

  void number() {
    double number = atof(previous().lexeme.c_str());
    emitConstant(number);
  }

  void grouping() {
    expression();
    consume(Token::TokenType::RIGHT_PAREN, std::string{kExpectRightParen});
  }

  void unary() {
    const Token& op = previous();
    parsePrecedence(Precedence::UNARY);
    switch (op.type) {
      case Token::TokenType::MINUS:
        chunk_.addCode(OpCode::NEGATE, op.line);
        break;
      default:
        return;  // Unreachable
    }
  }

  void binary() {
    const Token& op = previous();
    const auto rule = getRule(op);
    parsePrecedence((Precedence)(static_cast<int>(rule.precedence) + 1));

    switch (op.type) {
      case Token::TokenType::PLUS:
        chunk_.addCode(OpCode::ADD, op.line);
        break;
      case Token::TokenType::MINUS:
        chunk_.addCode(OpCode::SUBSTRACT, op.line);
        break;
      case Token::TokenType::STAR:
        chunk_.addCode(OpCode::MULTIPLY, op.line);
        break;
      case Token::TokenType::SLASH:
        chunk_.addCode(OpCode::DIVIDE, op.line);
        break;
      default:
        return;  // Unreachable
    }
  }

  const Token& current() const { return tokens_[current_]; }

  const Token& previous() const { return tokens_[current_ - 1]; }

  bool isAtEnd() const { return current().type == Token::TokenType::END; }

  const Token& advance() {
    if (!isAtEnd()) current_++;
    return previous();
  }

  void error(const Token& token, const std::string_view& message) {
    hadError_ = true;
    std::cout << "Error [line " << token.line << "] at " << token.lexeme << ": "
              << message << "\n";
    throw ParseError(std::string(message));
  }

  bool check(const Token::TokenType& type) const {
    return isAtEnd() ? false : current().type == type;
  }

  const Token& consume(Token::TokenType type, const std::string_view& message) {
    if (!check(type)) {
      error(current(), message);
    }
    return advance();
  }

  void synchronize() {
    advance();
    while (!isAtEnd()) {
      if (previous().type == Token::TokenType::SEMICOLON) return;
      switch (current().type) {
        case Token::TokenType::CLASS:
        case Token::TokenType::FOR:
        case Token::TokenType::FUN:
        case Token::TokenType::IF:
        case Token::TokenType::PRINT:
        case Token::TokenType::RETURN:
        case Token::TokenType::VAR:
        case Token::TokenType::WHILE:
          return;
        default:
          advance();
      }
    }
  }

  const ParseRule& getRule(const Token& token) {
    auto grouping = [this]() { this->grouping(); };
    auto unary = [this]() { this->unary(); };
    auto binary = [this]() { this->binary(); };
    auto number = [this]() { this->number(); };

    static const std::vector<ParseRule> rules = {
        {grouping, nullptr, Precedence::NONE},  // LEFT_PAREN
        {nullptr, nullptr, Precedence::NONE},   // RIGHT_PAREN
        {nullptr, nullptr, Precedence::NONE},   // LEFT_BRACE
        {nullptr, nullptr, Precedence::NONE},   // RIGHT_BRACE
        {nullptr, nullptr, Precedence::NONE},   // COMMA
        {nullptr, nullptr, Precedence::NONE},   // DOT
        {unary, binary, Precedence::TERM},      // MINUS
        {nullptr, binary, Precedence::TERM},    // PLUS
        {nullptr, nullptr, Precedence::NONE},   // COLON
        {nullptr, nullptr, Precedence::NONE},   // SEMICOLON
        {nullptr, binary, Precedence::FACTOR},  // SLASH
        {nullptr, binary, Precedence::FACTOR},  // STAR
        {nullptr, nullptr, Precedence::NONE},   // BANG
        {nullptr, nullptr, Precedence::NONE},   // EQUAL
        {nullptr, nullptr, Precedence::NONE},   // GREATER
        {nullptr, nullptr, Precedence::NONE},   // LESS
        {nullptr, nullptr, Precedence::NONE},   // QUESTION
        {nullptr, nullptr, Precedence::NONE},   // BANG_EQUAL
        {nullptr, nullptr, Precedence::NONE},   // EQUAL_EQUAL
        {nullptr, nullptr, Precedence::NONE},   // GREATER_EQUAL
        {nullptr, nullptr, Precedence::NONE},   // LESS_EQUAL
        {nullptr, nullptr, Precedence::NONE},   // MINUS_EQUAL
        {nullptr, nullptr, Precedence::NONE},   // PLUS_EQUAL
        {nullptr, nullptr, Precedence::NONE},   // SLASH_EQUAL
        {nullptr, nullptr, Precedence::NONE},   // STAR_EQUAL
        {nullptr, nullptr, Precedence::NONE},   // MINUS_MINUS
        {nullptr, nullptr, Precedence::NONE},   // PLUS_PLUS
        {nullptr, nullptr, Precedence::NONE},   // IDENTIFIER
        {nullptr, nullptr, Precedence::NONE},   // STRING
        {number, nullptr, Precedence::NONE},    // NUMBER
        {nullptr, nullptr, Precedence::NONE},   // AND
        {nullptr, nullptr, Precedence::NONE},   // CLASS
        {nullptr, nullptr, Precedence::NONE},   // ELSE
        {nullptr, nullptr, Precedence::NONE},   // FALSE
        {nullptr, nullptr, Precedence::NONE},   // FUN
        {nullptr, nullptr, Precedence::NONE},   // LAMBDA
        {nullptr, nullptr, Precedence::NONE},   // FOR
        {nullptr, nullptr, Precedence::NONE},   // IF
        {nullptr, nullptr, Precedence::NONE},   // NIL
        {nullptr, nullptr, Precedence::NONE},   // OR
        {nullptr, nullptr, Precedence::NONE},   // PRINT
        {nullptr, nullptr, Precedence::NONE},   // RETURN
        {nullptr, nullptr, Precedence::NONE},   // SUPER
        {nullptr, nullptr, Precedence::NONE},   // THIS
        {nullptr, nullptr, Precedence::NONE},   // TRUE
        {nullptr, nullptr, Precedence::NONE},   // VAR
        {nullptr, nullptr, Precedence::NONE},   // WHILE
        {nullptr, nullptr, Precedence::NONE},   // BREAK
        {nullptr, nullptr, Precedence::NONE},   // CONTINUE
        {nullptr, nullptr, Precedence::NONE},   // END
        {nullptr, nullptr, Precedence::NONE},   // ERROR
    };
    return rules[static_cast<int>(token.type)];
  }
  // std::unordered_map<Token::TokenType, ParseRule> rules {
  // };
};
}  // namespace lang
}  // namespace lox