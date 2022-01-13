#include "Parser.h"

#include <gflags/gflags.h>

#include "../RuntimeError.h"
#include "ParseError.h"

DECLARE_bool(debug);
constexpr std::string_view kExpectLeftParen = "Expect '(' after expression.";
constexpr std::string_view kExpectRightParen = "Expect ')' after expression.";
constexpr std::string_view kExpectLeftBrace = "Expect '{' after expression.";
constexpr std::string_view kExpectRightBrace = "Expect '}' after expression.";
constexpr std::string_view kExpectSemicolon = "Expect ';' after statement.";

namespace lox {
namespace compiler {

Closure Parser::run() {
  auto chunk = std::make_unique<Chunk>();
  try {
    while (!isAtEnd()) {
      declaration(*chunk, 0);
    }
    end(*chunk);

  } catch (ParseError& error) {
    hadError_ = true;
    std::cout << error.what();
    scanner_->synchronize();
  } catch (RuntimeError& error) {
    hadError_ = true;
    std::cout << error.what();
    scanner_->synchronize();
  }
  if (!hadError_) {
    chunk->scope.clear();
    chunk->upvalues.clear();
    auto func =
        std::make_shared<FunctionObject>(0, "<script>", std::move(chunk));
    return std::make_shared<ClosureObject>(std::move(func));
  }
  return nullptr;
}

void Parser::declaration(Chunk& chunk, int depth) {
  if (scanner_->match(Token::Type::VAR)) {
    variableDeclaration(chunk, depth);
  } else if (scanner_->match(Token::Type::FUN)) {
    functionDeclaration(chunk, depth);
  } else {
    statement(chunk, depth);
  }
}

void Parser::end(Chunk& chunk) { emitReturnNil(chunk); }

void Parser::variableDeclaration(Chunk& chunk, int depth) {
  const Token global = parseVariable("Expect variable name");
  declareVariable(chunk, global, depth);

  if (scanner_->match(Token::Type::EQUAL)) {
    expression(chunk, depth);
  } else {
    chunk.addCode(OpCode::NIL, global.line);
  }

  defineVariable(chunk, global, depth);
  scanner_->consume(Token::Type::SEMICOLON, kExpectSemicolon);
}

void Parser::functionDeclaration(Chunk& chunk, int depth) {
  const Token global = parseVariable("Expect variable name");
  declareVariable(chunk, global, depth);
  function(chunk, global.lexeme, depth);
  defineVariable(chunk, global, depth);
}

void Parser::function(Chunk& chunk, const std::string& name, int depth) {
  int line = scanner_->previous().line;
  auto function_chunk = std::make_unique<Chunk>();
  function_chunk->parent = &chunk;
  int scope = depth + 1;

  startScope(*function_chunk, scope);

  scanner_->consume(Token::Type::LEFT_PAREN, kExpectLeftParen);
  int arity = 0;
  if (!scanner_->check(Token::Type::RIGHT_PAREN)) {
    do {
      arity++;
      if (arity > 255) {
        parse_error(scanner_->current(),
                    "Function can't have more than 255 parameter");
      }
      auto constant = parseVariable("expect parameter name");
      declareVariable(*function_chunk, constant, scope);
      defineVariable(*function_chunk, constant, scope);
    } while (scanner_->match(Token::Type::COMMA));
  }
  scanner_->consume(Token::Type::RIGHT_PAREN, kExpectRightParen);
  scanner_->consume(Token::Type::LEFT_BRACE, kExpectLeftBrace);

  block(*function_chunk, scope);
  if (function_chunk->code.empty() ||
      function_chunk->code.back() != static_cast<uint8_t>(OpCode::RETURN)) {
    emitReturnNil(*function_chunk);
  }

  Function func =
      std::make_shared<FunctionObject>(arity, name, std::move(function_chunk));

  emitConstant(chunk, func, OpCode::CLOSURE, line);
  // for (auto& upvalue : chunk.upvalues) {
  //   chunk.addOperand(upvalue.isLocal ? 1 : 0);
  //   chunk.addOperand(upvalue.index);
  // }
  // endScope(chunk, scope);
}
void Parser::call(Chunk& chunk, int depth, bool canAssign) {
  uint8_t argCount = argumentList(chunk, depth);
  chunk.addCode(OpCode::CALL, scanner_->previous().line);
  chunk.addOperand(argCount);
}

uint8_t Parser::argumentList(Chunk& chunk, int depth) {
  uint8_t count = 0;
  if (!scanner_->check(Token::Type::RIGHT_PAREN)) {
    do {
      expression(chunk, depth);
      count++;
      if (count == 255) {
        parse_error(scanner_->current(),
                    "Function cant have more than 255 arguments");
      }
    } while (scanner_->match(Token::Type::COMMA));
  }
  scanner_->consume(Token::Type::RIGHT_PAREN, kExpectRightParen);
  return count;
}

void Parser::statement(Chunk& chunk, int depth) {
  if (scanner_->match(Token::Type::PRINT)) {
    printStatement(chunk, depth);
  } else if (scanner_->match(Token::Type::RETURN)) {
    returnStatement(chunk, depth);
  } else if (scanner_->match(Token::Type::WHILE)) {
    whileStatement(chunk, depth);
  } else if (scanner_->match(Token::Type::FOR)) {
    forStatement(chunk, depth);
  } else if (scanner_->match(Token::Type::IF)) {
    ifStatement(chunk, depth);
  } else if (scanner_->match(Token::Type::LEFT_BRACE)) {
    int scope = depth + 1;
    try {
      startScope(chunk, scope);
      block(chunk, scope);
      endScope(chunk, scope);
    } catch (ParseError& error) {
      endScope(chunk, scope);
      throw error;
    }
  } else {
    expressionStatement(chunk, depth);
  }
}

void Parser::printStatement(Chunk& chunk, int depth) {
  int line = scanner_->previous().line;
  expression(chunk, depth);
  chunk.addCode(OpCode::PRINT, line);
  chunk.addCode(OpCode::POP, line);
  scanner_->consume(Token::Type::SEMICOLON, kExpectSemicolon);
}

void Parser::block(Chunk& chunk, int depth) {
  while (!scanner_->check(Token::Type::RIGHT_BRACE)) {
    declaration(chunk, depth);
  }
  scanner_->consume(Token::Type::RIGHT_BRACE, kExpectRightBrace);
}

void Parser::expressionStatement(Chunk& chunk, int depth) {
  expression(chunk, depth);
  chunk.addCode(OpCode::POP, scanner_->previous().line);
  scanner_->consume(Token::Type::SEMICOLON, kExpectSemicolon);
}

void Parser::ifStatement(Chunk& chunk, int depth) {
  int line = scanner_->previous().line;
  scanner_->consume(Token::Type::LEFT_PAREN, kExpectLeftParen);
  expression(chunk, depth);
  scanner_->consume(Token::Type::RIGHT_PAREN, kExpectRightParen);

  int thenJump = emitJump(chunk, OpCode::JUMP_IF_FALSE, line);
  chunk.addCode(OpCode::POP, line);
  statement(chunk, depth);
  line = scanner_->previous().line;
  int elseJump = emitJump(chunk, OpCode::JUMP, line);
  patchJump(chunk, thenJump);
  chunk.addCode(OpCode::POP, line);

  if (scanner_->match(Token::Type::ELSE)) {
    statement(chunk, depth);
  }
  patchJump(chunk, elseJump);
}

void Parser::whileStatement(Chunk& chunk, int depth) {
  int line = scanner_->previous().line;
  int loopStart = chunk.code.size();
  scanner_->consume(Token::Type::LEFT_PAREN, kExpectLeftParen);
  expression(chunk, depth);
  scanner_->consume(Token::Type::RIGHT_PAREN, kExpectRightParen);

  int exitJump = emitJump(chunk, OpCode::JUMP_IF_FALSE, line);
  chunk.addCode(OpCode::POP, line);
  statement(chunk, depth);
  emitLoop(chunk, loopStart, line);

  patchJump(chunk, exitJump);
  chunk.addCode(OpCode::POP, line);
}

void Parser::forStatement(Chunk& chunk, int depth) {
  int line = scanner_->previous().line;
  int scope = depth + 1;
  startScope(chunk, scope);
  scanner_->consume(Token::Type::LEFT_PAREN, kExpectLeftParen);

  // init
  if (scanner_->match(Token::Type::SEMICOLON)) {
    // nil
  } else if (scanner_->match(Token::Type::VAR)) {
    variableDeclaration(chunk, scope);
  } else {
    expressionStatement(chunk, scope);
  }
  int loopStart = chunk.code.size();

  // condition
  int exitJump = -1;
  if (!scanner_->match(Token::Type::SEMICOLON)) {
    expression(chunk, scope);
    scanner_->consume(Token::Type::SEMICOLON, kExpectSemicolon);

    exitJump = emitJump(chunk, OpCode::JUMP_IF_FALSE, line);
    chunk.addCode(OpCode::POP, line);
  }

  // increment
  if (!scanner_->match(Token::Type::RIGHT_PAREN)) {
    int bodyJump = emitJump(chunk, OpCode::JUMP, line);

    int incrementStart = chunk.code.size();
    expression(chunk, scope);
    chunk.addCode(OpCode::POP, line);
    scanner_->consume(Token::Type::RIGHT_PAREN, kExpectRightParen);

    emitLoop(chunk, loopStart, line);
    loopStart = incrementStart;
    patchJump(chunk, bodyJump);
  }

  statement(chunk, scope);

  emitLoop(chunk, loopStart, line);

  if (exitJump != -1) {
    patchJump(chunk, exitJump);
    chunk.addCode(OpCode::POP, line);
  }
  endScope(chunk, scope);
}

void Parser::returnStatement(Chunk& chunk, int depth) {
  if (scanner_->match(Token::Type::SEMICOLON)) {
    emitReturnNil(chunk);
  } else {
    expression(chunk, depth);
    scanner_->consume(Token::Type::SEMICOLON, kExpectSemicolon);
    emitReturn(chunk);
  }
}

void Parser::and_(Chunk& chunk, int depth, bool canAssign) {
  int line = scanner_->previous().line;
  int endJump = emitJump(chunk, OpCode::JUMP_IF_FALSE, line);
  chunk.addCode(OpCode::POP, line);
  parsePrecedence(chunk, depth, Precedence::AND);
  patchJump(chunk, endJump);
}

void Parser::or_(Chunk& chunk, int depth, bool canAssign) {
  int line = scanner_->previous().line;
  int elseJump = emitJump(chunk, OpCode::JUMP_IF_FALSE, line);
  int endJump = emitJump(chunk, OpCode::JUMP, line);
  patchJump(chunk, elseJump);
  chunk.addCode(OpCode::POP, line);

  parsePrecedence(chunk, depth, Precedence::OR);
  patchJump(chunk, endJump);
}

void Parser::startScope(Chunk& chunk, int depth) {
  chunk.scope.push_scope(depth);
}

void Parser::endScope(Chunk& chunk, int depth) {
  auto removed_vars = chunk.scope.pop_scope(depth);
  int line = scanner_->previous().line;
  while (removed_vars--) {
    chunk.addCode(OpCode::POP, line);
  }
}
const Token& Parser::parseVariable(const std::string& error_message) {
  return scanner_->consume(Token::Type::IDENTIFIER, error_message);
}

void Parser::declareVariable(Chunk& chunk, const Token& name, int depth) {
  std::cout << "declare " << name.lexeme << " depth " << depth << "\n";
  if (depth == 0) {
    return;
  }
  chunk.scope.declare(name, depth);
}

void Parser::defineVariable(Chunk& chunk, const Token& name, int depth) {
  std::cout << "define " << name.lexeme << " depth " << depth << "\n";
  if (depth > 0) {
    chunk.scope.initialize(name, depth);
    return;
  }
  emitConstant(chunk, name.lexeme, OpCode::DEFINE_GLOBAL, name.line);
}

void Parser::expression(Chunk& chunk, int depth) {
  parsePrecedence(chunk, depth, Precedence::ASSIGNMENT);
}

void Parser::grouping(Chunk& chunk, int depth, bool canAssign) {
  expression(chunk, depth);
  scanner_->consume(Token::Type::RIGHT_PAREN, std::string{kExpectRightParen});
}

void Parser::unary(Chunk& chunk, int depth, bool canAssign) {
  const Token& op = scanner_->previous();
  parsePrecedence(chunk, depth, Precedence::UNARY);
  switch (op.type) {
    case Token::Type::MINUS:
      chunk.addCode(OpCode::NEGATE, op.line);
      break;
    case Token::Type::BANG:
      chunk.addCode(OpCode::NOT, op.line);
      break;
    default:
      return;  // Unreachable
  }
}
void Parser::binary(Chunk& chunk, int depth, bool canAssign) {
  const Token& op = scanner_->previous();
  const auto rule = getRule(op);
  parsePrecedence(chunk, depth,
                  (Precedence)(static_cast<int>(rule.precedence) + 1));

  switch (op.type) {
    case Token::Type::PLUS:
      chunk.addCode(OpCode::ADD, op.line);
      break;
    case Token::Type::MINUS:
      chunk.addCode(OpCode::SUBSTRACT, op.line);
      break;
    case Token::Type::STAR:
      chunk.addCode(OpCode::MULTIPLY, op.line);
      break;
    case Token::Type::SLASH:
      chunk.addCode(OpCode::DIVIDE, op.line);
      break;
    case Token::Type::GREATER:
      chunk.addCode(OpCode::GREATER, op.line);
      break;
    case Token::Type::LESS:
      chunk.addCode(OpCode::LESS, op.line);
      break;
    case Token::Type::BANG_EQUAL:
      chunk.addCode(OpCode::NOT_EQUAL, op.line);
      break;
    case Token::Type::EQUAL_EQUAL:
      chunk.addCode(OpCode::EQUAL, op.line);
      break;
    case Token::Type::GREATER_EQUAL:
      chunk.addCode(OpCode::GREATER_EQUAL, op.line);
      break;
    case Token::Type::LESS_EQUAL:
      chunk.addCode(OpCode::LESS_EQUAL, op.line);
      break;
    default:
      return;  // Unreachable
  }
}

void Parser::number(Chunk& chunk, int depth, bool canAssign) {
  double number = atof(scanner_->previous().lexeme.c_str());
  emitConstant(chunk, number, OpCode::CONSTANT, scanner_->previous().line);
}

void Parser::string(Chunk& chunk, int depth, bool canAssign) {
  emitConstant(chunk, scanner_->previous().lexeme, OpCode::CONSTANT,
               scanner_->previous().line);
}

void Parser::literal(Chunk& chunk, int depth, bool canAssign) {
  switch (scanner_->previous().type) {
    case Token::Type::FALSE:
      chunk.addCode(OpCode::FALSE, scanner_->previous().line);
      break;
    case Token::Type::TRUE:
      chunk.addCode(OpCode::TRUE, scanner_->previous().line);
      break;
    case Token::Type::NIL:
      chunk.addCode(OpCode::NIL, scanner_->previous().line);
      break;
    default:
      return;  // Unreachable
  }
}

void Parser::variable(Chunk& chunk, int depth, bool canAssign) {
  namedVariable(chunk, scanner_->previous(), canAssign, depth);
}

size_t Parser::resolveUpvalue(Chunk& chunk, const Token& name) {
  if (chunk.parent == nullptr) {
    return -1;
  }
  auto local = resolveLocal(*(chunk.parent), name);
  if (local != -1) {
    return addUpvalue(chunk, static_cast<uint8_t>(local), true);
  }

  int upvalue = resolveUpvalue(*(chunk.parent), name);
  if (upvalue != -1) {
    return addUpvalue(chunk, static_cast<uint8_t>(upvalue), false);
  }
  return -1;
}

int Parser::addUpvalue(Chunk& chunk, uint8_t index, bool isLocal) {
  for (size_t i = 0; i < chunk.upvalues.size(); i++) {
    if (chunk.upvalues[i].index == index &&
        chunk.upvalues[i].isLocal == isLocal) {
      return static_cast<int>(i);
    }
  }

  if (chunk.upvalues.size() == 255) {
    parse_error(scanner_->current(), "Too many closure variables in function.");
    return 0;
  }

  chunk.upvalues.emplace_back(Upvalue(index, isLocal));
  return chunk.upvalues.size() - 1;
}

void Parser::namedVariable(Chunk& chunk, const Token& token, bool canAssign,
                           int depth) {
  int offset = resolveLocal(chunk, token);
  int upvalue = resolveUpvalue(chunk, token);
  std::cout << "Variable " << token.lexeme << " local " << offset << " upvalue "
            << upvalue << "\n";

  OpCode get;
  OpCode set;
  if (offset != -1) {
    get = OpCode::GET_LOCAL;
    set = OpCode::SET_LOCAL;
    offset++;
  } else if (upvalue != -1) {
    get = OpCode::GET_UPVALUE;
    set = OpCode::SET_UPVALUE;
    offset = upvalue;
  } else {
    get = OpCode::GET_GLOBAL;
    set = OpCode::SET_GLOBAL;
    offset = chunk.addConstant(token.lexeme);
  }

  if (canAssign && scanner_->match(Token::Type::EQUAL)) {
    expression(chunk, depth);
    emitNamedVariable(chunk, set, offset, token.line);
  } else {
    emitNamedVariable(chunk, get, offset, token.line);
  }
}

void Parser::parsePrecedence(Chunk& chunk, int depth,
                             const Precedence& precedence) {
  scanner_->advance();
  const auto rule = getRule(scanner_->previous());
  if (rule.prefix == nullptr) {
    parse_error(scanner_->previous(), "Expected expression.");
    return;
  }

  bool canAssign = precedence <= Precedence::ASSIGNMENT;
  rule.prefix(chunk, depth, canAssign);

  while (precedence <= getRule(scanner_->current()).precedence) {
    scanner_->advance();
    const auto infixRule = getRule(scanner_->previous());
    if (infixRule.infix == nullptr) {
      parse_error(scanner_->previous(), "Expected infix expression.");
      return;
    }
    infixRule.infix(chunk, depth, canAssign);
  }
}
}  // namespace compiler
}  // namespace lox