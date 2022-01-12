#pragma once
#include <string>

#include "Chunk.h"
#include "Scanner.h"
#include "ScannerFactory.h"
#include "Scope.h"
#include "Value.h"
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
  std::function<void(Chunk& chunk, int depth, bool canAssign)> prefix;
  std::function<void(Chunk& chunk, int depth, bool canAssign)> infix;
  const Precedence precedence;
};

class Parser {
 public:
  Parser(const std::string& source, const std::string& scanner, Scope& scope)
      : scanner_{ScannerFactory::get(scanner)(source)}, scope_{scope} {}

  Function run();

 private:
  std::unique_ptr<Scanner> scanner_;
  Scope& scope_;
  bool hadError_{false};

  inline bool isAtEnd() const {
    return scanner_->current().type == Token::Type::END;
  }

  void declaration(Chunk& chunk, int depth);
  void end(Chunk& chunk);
  void variableDeclaration(Chunk& chunk, int depth);
  void functionDeclaration(Chunk& chunk, int depth);
  void statement(Chunk& chunk, int depth);
  void printStatement(Chunk& chunk, int depth);
  void block(Chunk& chunk, int depth);
  void expressionStatement(Chunk& chunk, int depth);
  void ifStatement(Chunk& chunk, int depth);
  void whileStatement(Chunk& chunk, int depth);
  void forStatement(Chunk& chunk, int depth);
  void expression(Chunk& chunk, int depth);
  void function(Chunk& chunk, const std::string& name, int depth);
  void grouping(Chunk& chunk, int depth, bool canAssign);
  void unary(Chunk& chunk, int depth, bool canAssign);
  void binary(Chunk& chunk, int depth, bool canAssign);
  void number(Chunk& chunk, int depth, bool canAssign);
  void string(Chunk& chunk, int depth, bool canAssign);
  void literal(Chunk& chunk, int depth, bool canAssign);
  void variable(Chunk& chunk, int depth, bool canAssign);
  void and_(Chunk& chunk, int depth, bool canAssign);
  void or_(Chunk& chunk, int depth, bool canAssign);
  void call(Chunk& chunk, int depth, bool canAssign);

  const Token& parseVariable(const std::string& error_message);
  void declareVariable(const Token& name, int depth);
  void defineVariable(Chunk& chunk, const Token& name, int depth);
  void namedVariable(Chunk& chunk, const Token& token, bool canAssign,
                     int depth);
  inline size_t resolveLocal(const Token& name) { return scope_.find(name); }
  uint8_t argumentList(Chunk& chunk, int depth);

  void parsePrecedence(Chunk& chunk, int depth, const Precedence& precedence);
  void startScope(int depth);
  void endScope(Chunk& chunk, int depth);

  inline void emitReturn(Chunk& chunk) {
    chunk.addCode(OpCode::RETURN, scanner_->previous().line);
  }
  inline void emitConstant(Chunk& chunk, const Value& constant,
                           const OpCode& code, int line) {
    auto offset = chunk.addConstant(constant);
    chunk.addCode(code, line);
    chunk.addOperand(offset);
  }
  inline int emitJump(Chunk& chunk, const OpCode& code, int line) {
    chunk.addCode(code, line);
    chunk.addOperand(0xff);
    chunk.addOperand(0xff);
    return chunk.code.size() - 2;
  }
  inline void patchJump(Chunk& chunk, int offset) {
    int jump = chunk.code.size() - offset - 2;

    if (jump > std::numeric_limits<uint16_t>::max()) {
      parse_error(scanner_->previous(), "loop body too large");
    }

    chunk.code[offset] = (jump >> 8) & 0xff;
    chunk.code[offset + 1] = jump & 0xff;
  }

  inline void emitLoop(Chunk& chunk, int loopStart, int line) {
    chunk.addCode(OpCode::LOOP, line);
    int offset = chunk.code.size() - loopStart + 2;
    if (offset > std::numeric_limits<uint16_t>::max()) {
      parse_error(scanner_->previous(), "loop body too large");
    }
    chunk.addOperand((offset >> 8) & 0xff);
    chunk.addOperand(offset & 0xff);
  }

  static inline void parse_error(const Token& token,
                                 const std::string_view& message) {
    std::stringstream ss;
    ss << "ParseError [line " << token.line << "]: " << message << " [at "
       << token.lexeme << "]\n";
    throw ParseError(ss.str());
  }

  const ParseRule& getRule(const Token& token) {
    auto grouping = [this](Chunk& chunk, int depth, bool canAssign) {
      this->grouping(chunk, depth, canAssign);
    };
    auto unary = [this](Chunk& chunk, int depth, bool canAssign) {
      this->unary(chunk, depth, canAssign);
    };
    auto binary = [this](Chunk& chunk, int depth, bool canAssign) {
      this->binary(chunk, depth, canAssign);
    };
    auto number = [this](Chunk& chunk, int depth, bool canAssign) {
      this->number(chunk, depth, canAssign);
    };
    auto literal = [this](Chunk& chunk, int depth, bool canAssign) {
      this->literal(chunk, depth, canAssign);
    };
    auto string = [this](Chunk& chunk, int depth, bool canAssign) {
      this->string(chunk, depth, canAssign);
    };
    auto variable = [this](Chunk& chunk, int depth, bool canAssign) {
      this->variable(chunk, depth, canAssign);
    };
    auto and_ = [this](Chunk& chunk, int depth, bool canAssign) {
      this->and_(chunk, depth, canAssign);
    };
    auto or_ = [this](Chunk& chunk, int depth, bool canAssign) {
      this->or_(chunk, depth, canAssign);
    };
    auto call = [this](Chunk& chunk, int depth, bool canAssign) {
      this->call(chunk, depth, canAssign);
    };

    static const std::vector<ParseRule> rules = {
        {grouping, call, Precedence::CALL},         // LEFT_PAREN
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
        {nullptr, and_, Precedence::AND},           // AND
        {nullptr, nullptr, Precedence::NONE},       // CLASS
        {nullptr, nullptr, Precedence::NONE},       // ELSE
        {literal, nullptr, Precedence::NONE},       // FALSE
        {nullptr, nullptr, Precedence::NONE},       // FUN
        {nullptr, nullptr, Precedence::NONE},       // LAMBDA
        {nullptr, nullptr, Precedence::NONE},       // FOR
        {nullptr, nullptr, Precedence::NONE},       // IF
        {literal, nullptr, Precedence::NONE},       // NIL
        {nullptr, or_, Precedence::OR},             // OR
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