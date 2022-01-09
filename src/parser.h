#pragma once

#include <stdint.h>

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "chunk.h"
#include "debug.h"
#include "scope.h"
#include "token.h"
#include "value.h"

DECLARE_bool(debug);

template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

namespace lox {
namespace lang {
constexpr std::string_view kExpectRightParen = "Expect ')' after expression.";
constexpr std::string_view kExpectRightBrace = "Expect '}' after expression.";
constexpr std::string_view kExpectSemicolon = "Expect ';' after statement.";
class Parser;

typedef void (Parser::*ParseFn)();

struct ParseError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

class Parser {
 public:
  Parser(const std::vector<Token>& tokens, Chunk& chunk, Scope& scope)
      : tokens_{tokens}, chunk_{chunk}, scope_{scope}, current_{0} {}

  bool parse() {
    try {
      while (!isAtEnd()) {
        declaration();
      }
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
  Scope& scope_;
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
    std::function<void(bool canAssign)> prefix;
    std::function<void(bool canAssign)> infix;
    const Precedence precedence;
  };

  void emitByte(uint8_t byte) {}

  void emitConstant(const Value& constant, const OpCode& code, int line) {
    auto offset = chunk_.addConstant(constant);
    chunk_.addCode(code, line);
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

    bool canAssign = precedence <= Precedence::ASSIGNMENT;
    rule.prefix(canAssign);

    while (precedence <= getRule(current()).precedence) {
      advance();
      const auto infixRule = getRule(previous());
      if (infixRule.infix == nullptr) {
        error(previous(), "Expected infix expression.");
        return;
      }
      infixRule.infix(canAssign);
    }
  }

  void declaration() {
    if (match(Token::TokenType::VAR)) {
      variableDeclaration();
    } else {
      statement();
    }
  }

  void variableDeclaration() {
    const auto& global = parseVariable("Expect variable name");
    declareVariable();

    if (match(Token::TokenType::EQUAL)) {
      expression();
    } else {
      chunk_.addCode(OpCode::NIL, global.line);
    }
    defineVariable(global);
    consume(Token::TokenType::SEMICOLON, kExpectSemicolon);
  }

  const Token& parseVariable(const std::string& error_message) {
    return consume(Token::TokenType::IDENTIFIER, error_message);
  }

  void defineVariable(const Token& var) {
    if (scope_.depth() > 0) {
      scope_.markInitialized();
      return;
    }
    emitConstant(var.lexeme, OpCode::DEFINE_GLOBAL, var.line);
  }

  void declareVariable() {
    if (scope_.depth() == 0) return;
    addLocal(previous());
  }

  void addLocal(const Token& name) {
    try {
      scope_.addLocal(name);

    } catch (std::runtime_error&) {
      error(name, "Variable already defined.");
    }
  }

  void statement() {
    if (match(Token::TokenType::PRINT)) {
      printStatement();
    } else if (match(Token::TokenType::LEFT_BRACE)) {
      try {
        beginScope();
        block();
        endScope();
      } catch (ParseError& error) {
        endScope();
        throw error;
      }
    } else {
      expressionStatement();
    }
  }

  void beginScope() { scope_.incrementDepth(); }
  void endScope() {
    auto removed_vars = scope_.decrementDepth();
    int line = previous().line;
    while (removed_vars--) {
      chunk_.addCode(OpCode::POP, line);
    }
  }

  void printStatement() {
    int line = previous().line;
    expression();
    chunk_.addCode(OpCode::PRINT, line);
    consume(Token::TokenType::SEMICOLON, kExpectSemicolon);
  }

  void block() {
    while (!check(Token::TokenType::RIGHT_BRACE)) {
      declaration();
    }
    consume(Token::TokenType::RIGHT_BRACE, kExpectRightBrace);
  }

  void expressionStatement() {
    expression();
    chunk_.addCode(OpCode::POP, previous().line);
    consume(Token::TokenType::SEMICOLON, kExpectSemicolon);
  }

  void expression() { parsePrecedence(Precedence::ASSIGNMENT); }

  void number(bool canAssign) {
    double number = atof(previous().lexeme.c_str());
    emitConstant(number, OpCode::CONSTANT, previous().line);
  }

  void string(bool canAssign) {
    emitConstant(previous().lexeme, OpCode::CONSTANT, previous().line);
  }
  void variable(bool canAssign) { namedVariable(previous(), canAssign); }

  void namedVariable(const Token& token, bool canAssign) {
    int offset = resolveLocal(token);

    if (canAssign && match(Token::TokenType::EQUAL)) {
      expression();
      if (offset != -1) {
        chunk_.addCode(OpCode::SET_LOCAL, token.line);
        chunk_.addOperand(static_cast<uint8_t>(offset));
      } else {
        emitConstant(token.lexeme, OpCode::SET_GLOBAL, token.line);
      }
    } else {
      if (offset != -1) {
        chunk_.addCode(OpCode::GET_LOCAL, token.line);
        chunk_.addOperand(static_cast<uint8_t>(offset));
      } else {
        emitConstant(token.lexeme, OpCode::GET_GLOBAL, token.line);
      }
    }
  }

  inline size_t resolveLocal(const Token& name) {
    try {
      return scope_.find(name);
    } catch (std::runtime_error&) {
      error(previous(), "Uninitialized variable");
    }
    return -1;  // unreachable
  }

  void literal(bool canAssign) {
    switch (previous().type) {
      case Token::TokenType::FALSE:
        chunk_.addCode(OpCode::FALSE, previous().line);
        break;
      case Token::TokenType::TRUE:
        chunk_.addCode(OpCode::TRUE, previous().line);
        break;
      case Token::TokenType::NIL:
        chunk_.addCode(OpCode::NIL, previous().line);
        break;
      default:
        return;  // Unreachable
    }
  }

  void grouping(bool canAssign) {
    expression();
    consume(Token::TokenType::RIGHT_PAREN, std::string{kExpectRightParen});
  }

  void unary(bool canAssign) {
    const Token& op = previous();
    parsePrecedence(Precedence::UNARY);
    switch (op.type) {
      case Token::TokenType::MINUS:
        chunk_.addCode(OpCode::NEGATE, op.line);
        break;
      case Token::TokenType::BANG:
        chunk_.addCode(OpCode::NOT, op.line);
        break;
      default:
        return;  // Unreachable
    }
  }

  void binary(bool canAssign) {
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
      case Token::TokenType::GREATER:
        chunk_.addCode(OpCode::GREATER, op.line);
        break;
      case Token::TokenType::LESS:
        chunk_.addCode(OpCode::LESS, op.line);
        break;
      case Token::TokenType::BANG_EQUAL:
        chunk_.addCode(OpCode::NOT_EQUAL, op.line);
        break;
      case Token::TokenType::EQUAL_EQUAL:
        chunk_.addCode(OpCode::EQUAL, op.line);
        break;
      case Token::TokenType::GREATER_EQUAL:
        chunk_.addCode(OpCode::GREATER_EQUAL, op.line);
        break;
      case Token::TokenType::LESS_EQUAL:
        chunk_.addCode(OpCode::LESS_EQUAL, op.line);
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

  bool match(const Token::TokenType& type) {
    if (!check(type)) {
      return false;
    }
    advance();
    return true;
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
    auto grouping = [this](bool canAssign) { this->grouping(canAssign); };
    auto unary = [this](bool canAssign) { this->unary(canAssign); };
    auto binary = [this](bool canAssign) { this->binary(canAssign); };
    auto number = [this](bool canAssign) { this->number(canAssign); };
    auto literal = [this](bool canAssign) { this->literal(canAssign); };
    auto string = [this](bool canAssign) { this->string(canAssign); };
    auto variable = [this](bool canAssign) { this->variable(canAssign); };

    static const std::vector<ParseRule> rules = {
        {grouping, nullptr, Precedence::NONE},      // LEFT_PAREN
        {nullptr, nullptr, Precedence::NONE},       // RIGHT_PAREN
        {nullptr, nullptr, Precedence::NONE},       // LEFT_BRACE
        {nullptr, nullptr, Precedence::NONE},       // RIGHT_BRACE
        {nullptr, nullptr, Precedence::NONE},       // COMMA
        {nullptr, nullptr, Precedence::NONE},       // DOT
        {unary, binary, Precedence::TERM},          // MINUS
        {nullptr, binary, Precedence::TERM},        // PLUS
        {nullptr, nullptr, Precedence::NONE},       // COLON
        {nullptr, nullptr, Precedence::NONE},       // SEMICOLON
        {nullptr, binary, Precedence::FACTOR},      // SLASH
        {nullptr, binary, Precedence::FACTOR},      // STAR
        {unary, nullptr, Precedence::NONE},         // BANG
        {nullptr, nullptr, Precedence::NONE},       // EQUAL
        {nullptr, binary, Precedence::COMPARISON},  // GREATER
        {nullptr, binary, Precedence::COMPARISON},  // LESS
        {nullptr, nullptr, Precedence::NONE},       // QUESTION
        {nullptr, binary, Precedence::EQUALITY},    // BANG_EQUAL
        {nullptr, binary, Precedence::EQUALITY},    // EQUAL_EQUAL
        {nullptr, binary, Precedence::COMPARISON},  // GREATER_EQUAL
        {nullptr, binary, Precedence::COMPARISON},  // LESS_EQUAL
        {nullptr, nullptr, Precedence::NONE},       // MINUS_EQUAL
        {nullptr, nullptr, Precedence::NONE},       // PLUS_EQUAL
        {nullptr, nullptr, Precedence::NONE},       // SLASH_EQUAL
        {nullptr, nullptr, Precedence::NONE},       // STAR_EQUAL
        {nullptr, nullptr, Precedence::NONE},       // MINUS_MINUS
        {nullptr, nullptr, Precedence::NONE},       // PLUS_PLUS
        {variable, nullptr, Precedence::NONE},      // IDENTIFIER
        {string, nullptr, Precedence::NONE},        // STRING
        {number, nullptr, Precedence::NONE},        // NUMBER
        {nullptr, nullptr, Precedence::NONE},       // AND
        {nullptr, nullptr, Precedence::NONE},       // CLASS
        {nullptr, nullptr, Precedence::NONE},       // ELSE
        {literal, nullptr, Precedence::NONE},       // FALSE
        {nullptr, nullptr, Precedence::NONE},       // FUN
        {nullptr, nullptr, Precedence::NONE},       // LAMBDA
        {nullptr, nullptr, Precedence::NONE},       // FOR
        {nullptr, nullptr, Precedence::NONE},       // IF
        {literal, nullptr, Precedence::NONE},       // NIL
        {nullptr, nullptr, Precedence::NONE},       // OR
        {nullptr, nullptr, Precedence::NONE},       // PRINT
        {nullptr, nullptr, Precedence::NONE},       // RETURN
        {nullptr, nullptr, Precedence::NONE},       // SUPER
        {nullptr, nullptr, Precedence::NONE},       // THIS
        {literal, nullptr, Precedence::NONE},       // TRUE;
        {nullptr, nullptr, Precedence::NONE},       // VAR
        {nullptr, nullptr, Precedence::NONE},       // WHILE
        {nullptr, nullptr, Precedence::NONE},       // BREAK
        {nullptr, nullptr, Precedence::NONE},       // CONTINUE
        {nullptr, nullptr, Precedence::NONE},       // END
        {nullptr, nullptr, Precedence::NONE},       // ERROR
    };
    return rules[static_cast<int>(token.type)];
  }
  // std::unordered_map<Token::TokenType, ParseRule> rules {
  // };
};
}  // namespace lang
}  // namespace lox