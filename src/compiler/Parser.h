#pragma once
#include <string>

#include "Chunk.h"
#include "Scanner.h"
#include "ScannerFactory.h"
#include "Scope.h"
#include "debug.h"

namespace lox {
namespace compiler {

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
  std::function<void(int depth, bool canAssign)> prefix;
  std::function<void(int depth, bool canAssign)> infix;
  const Precedence precedence;
};

class Parser {
 public:
  Parser(const std::string& source, const std::string& scanner, Chunk& chunk,
         Scope& scope)
      : scanner_{ScannerFactory::get(scanner)(source)},
        // scanner_{std::make_unique<ReadByOneScanner>(source)},
        chunk_{chunk},
        scope_{scope} {}

  bool run();

 private:
  std::unique_ptr<Scanner> scanner_;
  Chunk& chunk_;
  Scope& scope_;
  bool hadError_{false};

  inline const Token& current() const { return scanner_->current(); }
  inline const Token& previous() const { return scanner_->previous(); }
  inline const Token& advance() { return scanner_->advance(); }
  inline bool isAtEnd() const {
    return scanner_->current().type == Token::Type::END;
  }

  void declaration(int depth);
  void end();
  void variableDeclaration(int depth);
  void statement(int depth);
  void printStatement(int depth);
  void block(int depth);
  void expressionStatement(int depth);
  void expression(int depth);
  void grouping(int depth, bool canAssign);
  void unary(int depth, bool canAssign);
  void binary(int depth, bool canAssign);
  void number(int depth, bool canAssign);
  void string(int depth, bool canAssign);
  void literal(int depth, bool canAssign);
  void variable(int depth, bool canAssign);

  const Token& parseVariable(const std::string& error_message);
  void declareVariable(const Token& name, int depth);
  void defineVariable(const Token& name, int depth);
  void namedVariable(const Token& token, bool canAssign, int depth);
  inline size_t resolveLocal(const Token& name) { return scope_.find(name); }

  void parsePrecedence(int depth, const Precedence& precedence);
  void endScope();

  inline void emitReturn() { chunk_.addCode(OpCode::RETURN, previous().line); }
  inline void emitConstant(const Value& constant, const OpCode& code,
                           int line) {
    auto offset = chunk_.addConstant(constant);
    chunk_.addCode(code, line);
    chunk_.addOperand(offset);
  }

  static inline void parse_error(const Token& token,
                                 const std::string_view& message) {
    std::stringstream ss;
    ss << "ParseError [line " << token.line << "]: " << message << " [at "
       << token.lexeme << "]\n";
    throw ParseError(ss.str());
  }

  const ParseRule& getRule(const Token& token) {
    auto grouping = [this](int depth, bool canAssign) {
      this->grouping(depth, canAssign);
    };
    auto unary = [this](int depth, bool canAssign) {
      this->unary(depth, canAssign);
    };
    auto binary = [this](int depth, bool canAssign) {
      this->binary(depth, canAssign);
    };
    auto number = [this](int depth, bool canAssign) {
      this->number(depth, canAssign);
    };
    auto literal = [this](int depth, bool canAssign) {
      this->literal(depth, canAssign);
    };
    auto string = [this](int depth, bool canAssign) {
      this->string(depth, canAssign);
    };
    auto variable = [this](int depth, bool canAssign) {
      this->variable(depth, canAssign);
    };

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
};

}  // namespace compiler
}  // namespace lox