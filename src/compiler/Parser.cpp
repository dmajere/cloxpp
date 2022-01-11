#include "Parser.h"

#include <gflags/gflags.h>

#include "../RuntimeError.h"
#include "ParseError.h"

DECLARE_bool(debug);
constexpr std::string_view kExpectLeftParen = "Expect '(' after expression.";
constexpr std::string_view kExpectRightParen = "Expect ')' after expression.";
constexpr std::string_view kExpectRightBrace = "Expect '}' after expression.";
constexpr std::string_view kExpectSemicolon = "Expect ';' after statement.";

namespace lox {
namespace compiler {
bool Parser::run() {
  try {
    while (!isAtEnd()) {
      declaration(0);
    }
    end();

    if (FLAGS_debug && !hadError_) {
      Disassembler::dis(chunk_, "Compiled chunk");
    }
  } catch (ParseError& error) {
    hadError_ = true;
    std::cout << error.what();
    scanner_->synchronize();
  } catch (RuntimeError& error) {
    hadError_ = true;
    std::cout << error.what();
    scanner_->synchronize();
  }
  return !hadError_;
}

void Parser::declaration(int depth) {
  if (scanner_->match(Token::Type::VAR)) {
    variableDeclaration(depth);
  } else {
    statement(depth);
  }
}

void Parser::end() { emitReturn(); }

void Parser::variableDeclaration(int depth) {
  const Token global = parseVariable("Expect variable name");
  declareVariable(global, depth);

  if (scanner_->match(Token::Type::EQUAL)) {
    expression(depth);
  } else {
    chunk_.addCode(OpCode::NIL, global.line);
  }

  defineVariable(global, depth);
  scanner_->consume(Token::Type::SEMICOLON, kExpectSemicolon);
}

void Parser::statement(int depth) {
  if (scanner_->match(Token::Type::PRINT)) {
    printStatement(depth);
  } else if (scanner_->match(Token::Type::WHILE)) {
    whileStatement(depth);
  } else if (scanner_->match(Token::Type::FOR)) {
    forStatement(depth);
  } else if (scanner_->match(Token::Type::IF)) {
    ifStatement(depth);
  } else if (scanner_->match(Token::Type::LEFT_BRACE)) {
    int scope = depth + 1;
    try {
      startScope(scope);
      block(scope);
      endScope(scope);
    } catch (ParseError& error) {
      endScope(scope);
      throw error;
    }
  } else {
    expressionStatement(depth);
  }
}

void Parser::printStatement(int depth) {
  int line = scanner_->previous().line;
  expression(depth);
  chunk_.addCode(OpCode::PRINT, line);
  chunk_.addCode(OpCode::POP, line);
  scanner_->consume(Token::Type::SEMICOLON, kExpectSemicolon);
}

void Parser::block(int depth) {
  while (!scanner_->check(Token::Type::RIGHT_BRACE)) {
    declaration(depth);
  }
  scanner_->consume(Token::Type::RIGHT_BRACE, kExpectRightBrace);
}

void Parser::expressionStatement(int depth) {
  expression(depth);
  chunk_.addCode(OpCode::POP, scanner_->previous().line);
  scanner_->consume(Token::Type::SEMICOLON, kExpectSemicolon);
}

void Parser::ifStatement(int depth) {
  int line = scanner_->previous().line;
  scanner_->consume(Token::Type::LEFT_PAREN, kExpectLeftParen);
  expression(depth);
  scanner_->consume(Token::Type::RIGHT_PAREN, kExpectRightParen);

  int thenJump = emitJump(OpCode::JUMP_IF_FALSE, line);
  chunk_.addCode(OpCode::POP, line);
  statement(depth);
  line = scanner_->previous().line;
  int elseJump = emitJump(OpCode::JUMP, line);
  patchJump(thenJump);
  chunk_.addCode(OpCode::POP, line);

  if (scanner_->match(Token::Type::ELSE)) {
    statement(depth);
  }
  patchJump(elseJump);
}

void Parser::whileStatement(int depth) {
  int line = scanner_->previous().line;
  int loopStart = chunk_.code.size();
  scanner_->consume(Token::Type::LEFT_PAREN, kExpectLeftParen);
  expression(depth);
  scanner_->consume(Token::Type::RIGHT_PAREN, kExpectRightParen);

  int exitJump = emitJump(OpCode::JUMP_IF_FALSE, line);
  chunk_.addCode(OpCode::POP, line);
  statement(depth);
  emitLoop(loopStart, line);

  patchJump(exitJump);
  chunk_.addCode(OpCode::POP, line);
}

void Parser::forStatement(int depth) {
  int line = scanner_->previous().line;
  int scope = depth + 1;
  startScope(scope);
  scanner_->consume(Token::Type::LEFT_PAREN, kExpectLeftParen);

  // init
  if (scanner_->match(Token::Type::SEMICOLON)) {
    // nil
  } else if (scanner_->match(Token::Type::VAR)) {
    variableDeclaration(scope);
  } else {
    expressionStatement(scope);
  }
  int loopStart = chunk_.code.size();

  // condition
  int exitJump = -1;
  if (!scanner_->match(Token::Type::SEMICOLON)) {
    expression(scope);
    scanner_->consume(Token::Type::SEMICOLON, kExpectSemicolon);

    exitJump = emitJump(OpCode::JUMP_IF_FALSE, line);
    chunk_.addCode(OpCode::POP, line);
  }

  // increment
  if (!scanner_->match(Token::Type::RIGHT_PAREN)) {
    int bodyJump = emitJump(OpCode::JUMP, line);

    int incrementStart = chunk_.code.size();
    expression(scope);
    chunk_.addCode(OpCode::POP, line);
    scanner_->consume(Token::Type::RIGHT_PAREN, kExpectRightParen);

    emitLoop(loopStart, line);
    loopStart = incrementStart;
    patchJump(bodyJump);
  }

  statement(scope);

  emitLoop(loopStart, line);

  if (exitJump != -1) {
    patchJump(exitJump);
    chunk_.addCode(OpCode::POP, line);
  }
  endScope(scope);
}

void Parser::and_(int depth, bool canAssign) {
  int line = scanner_->previous().line;
  int endJump = emitJump(OpCode::JUMP_IF_FALSE, line);
  chunk_.addCode(OpCode::POP, line);
  parsePrecedence(depth, Precedence::AND);
  patchJump(endJump);
}

void Parser::or_(int depth, bool canAssign) {
  int line = scanner_->previous().line;
  int elseJump = emitJump(OpCode::JUMP_IF_FALSE, line);
  int endJump = emitJump(OpCode::JUMP, line);
  patchJump(elseJump);
  chunk_.addCode(OpCode::POP, line);

  parsePrecedence(depth, Precedence::OR);
  patchJump(endJump);
}

void Parser::startScope(int depth) { scope_.push_scope(depth); }

void Parser::endScope(int depth) {
  auto removed_vars = scope_.pop_scope(depth);
  int line = scanner_->previous().line;
  while (removed_vars--) {
    chunk_.addCode(OpCode::POP, line);
  }
}
const Token& Parser::parseVariable(const std::string& error_message) {
  return scanner_->consume(Token::Type::IDENTIFIER, error_message);
}

void Parser::declareVariable(const Token& name, int depth) {
  if (depth == 0) {
    return;
  }
  scope_.declare(name, depth);
}

void Parser::defineVariable(const Token& name, int depth) {
  if (depth > 0) {
    scope_.initialize(name, depth);
    return;
  }
  emitConstant(name.lexeme, OpCode::DEFINE_GLOBAL, name.line);
}

void Parser::expression(int depth) {
  parsePrecedence(depth, Precedence::ASSIGNMENT);
}

void Parser::grouping(int depth, bool canAssign) {
  expression(depth);
  scanner_->consume(Token::Type::RIGHT_PAREN, std::string{kExpectRightParen});
}

void Parser::unary(int depth, bool canAssign) {
  const Token& op = scanner_->previous();
  parsePrecedence(depth, Precedence::UNARY);
  switch (op.type) {
    case Token::Type::MINUS:
      chunk_.addCode(OpCode::NEGATE, op.line);
      break;
    case Token::Type::BANG:
      chunk_.addCode(OpCode::NOT, op.line);
      break;
    default:
      return;  // Unreachable
  }
}
void Parser::binary(int depth, bool canAssign) {
  const Token& op = scanner_->previous();
  const auto rule = getRule(op);
  parsePrecedence(depth, (Precedence)(static_cast<int>(rule.precedence) + 1));

  switch (op.type) {
    case Token::Type::PLUS:
      chunk_.addCode(OpCode::ADD, op.line);
      break;
    case Token::Type::MINUS:
      chunk_.addCode(OpCode::SUBSTRACT, op.line);
      break;
    case Token::Type::STAR:
      chunk_.addCode(OpCode::MULTIPLY, op.line);
      break;
    case Token::Type::SLASH:
      chunk_.addCode(OpCode::DIVIDE, op.line);
      break;
    case Token::Type::GREATER:
      chunk_.addCode(OpCode::GREATER, op.line);
      break;
    case Token::Type::LESS:
      chunk_.addCode(OpCode::LESS, op.line);
      break;
    case Token::Type::BANG_EQUAL:
      chunk_.addCode(OpCode::NOT_EQUAL, op.line);
      break;
    case Token::Type::EQUAL_EQUAL:
      chunk_.addCode(OpCode::EQUAL, op.line);
      break;
    case Token::Type::GREATER_EQUAL:
      chunk_.addCode(OpCode::GREATER_EQUAL, op.line);
      break;
    case Token::Type::LESS_EQUAL:
      chunk_.addCode(OpCode::LESS_EQUAL, op.line);
      break;
    default:
      return;  // Unreachable
  }
}

void Parser::number(int depth, bool canAssign) {
  double number = atof(scanner_->previous().lexeme.c_str());
  emitConstant(number, OpCode::CONSTANT, scanner_->previous().line);
}

void Parser::string(int depth, bool canAssign) {
  emitConstant(scanner_->previous().lexeme, OpCode::CONSTANT,
               scanner_->previous().line);
}

void Parser::literal(int depth, bool canAssign) {
  switch (scanner_->previous().type) {
    case Token::Type::FALSE:
      chunk_.addCode(OpCode::FALSE, scanner_->previous().line);
      break;
    case Token::Type::TRUE:
      chunk_.addCode(OpCode::TRUE, scanner_->previous().line);
      break;
    case Token::Type::NIL:
      chunk_.addCode(OpCode::NIL, scanner_->previous().line);
      break;
    default:
      return;  // Unreachable
  }
}
void Parser::variable(int depth, bool canAssign) {
  namedVariable(scanner_->previous(), canAssign, depth);
}

void Parser::namedVariable(const Token& token, bool canAssign, int depth) {
  int offset = resolveLocal(token);

  if (canAssign && scanner_->match(Token::Type::EQUAL)) {
    expression(depth);
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

void Parser::parsePrecedence(int depth, const Precedence& precedence) {
  scanner_->advance();
  const auto rule = getRule(scanner_->previous());
  if (rule.prefix == nullptr) {
    parse_error(scanner_->previous(), "Expected expression.");
    return;
  }

  bool canAssign = precedence <= Precedence::ASSIGNMENT;
  rule.prefix(depth, canAssign);

  while (precedence <= getRule(current()).precedence) {
    scanner_->advance();
    const auto infixRule = getRule(scanner_->previous());
    if (infixRule.infix == nullptr) {
      parse_error(scanner_->previous(), "Expected infix expression.");
      return;
    }
    infixRule.infix(depth, canAssign);
  }
}
}  // namespace compiler
}  // namespace lox