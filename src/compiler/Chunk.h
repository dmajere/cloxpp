#pragma once
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Scope.h"
#include "Value.h"

namespace lox {
namespace compiler {

const std::vector<std::string> codes{
    "CONSTANT",      "NIL",          "TRUE",         "FALSE",
    "RETURN",        "NEGATE",       "ADD",          "SUBSTRACT",
    "MULTIPLY",      "DIVIDE",       "NOT",          "EQUAL",
    "GREATER",       "LESS",         "NOT_EQUAL",    "GREATER_EQUAL",
    "LESS_EQUAL",    "PRINT",        "POP",          "DEFINE_GLOBAL",
    "GET_GLOBAL",    "SET_GLOBAL",   "GET_LOCAL",    "SET_LOCAL",
    "JUMP_IF_FALSE", "JUMP",         "LOOP",         "CALL",
    "CLOSURE",       "SET_UPVALUE",  "GET_UPVALUE",  "CLOSE_UPVALUE",
    "CLASS",         "SET_PROPERTY", "GET_PROPERTY", "METHOD",
    "INVOKE",        "INHERIT",      "SUPER_INVOKE"};

enum class OpCode {
  CONSTANT,
  NIL,
  TRUE,
  FALSE,
  RETURN,
  NEGATE,
  ADD,
  SUBSTRACT,
  MULTIPLY,
  DIVIDE,
  NOT,
  EQUAL,
  GREATER,
  LESS,
  NOT_EQUAL,
  GREATER_EQUAL,
  LESS_EQUAL,
  PRINT,
  POP,
  DEFINE_GLOBAL,
  GET_GLOBAL,
  SET_GLOBAL,
  GET_LOCAL,
  SET_LOCAL,
  JUMP_IF_FALSE,
  JUMP,
  LOOP,
  CALL,
  CLOSURE,
  SET_UPVALUE,
  GET_UPVALUE,
  CLOSE_UPVALUE,
  CLASS,
  SET_PROPERTY,
  GET_PROPERTY,
  METHOD,
  INVOKE,
  INHERIT,
  GET_SUPER,
  SUPER_INVOKE,
};

class Upvalue {
 public:
  uint8_t index;
  bool isLocal;
  explicit Upvalue(uint8_t index, bool isLocal)
      : index(index), isLocal(isLocal) {}
};

struct Chunk {
  enum class Type {
    NONE,
    CLASS,
  };
  std::vector<uint8_t> code;
  std::vector<Value> constants;
  std::vector<int> lines;
  Chunk* parent = nullptr;
  Scope scope;
  std::vector<Upvalue> upvalues;
  Type type{Type::NONE};
  Type enclosingType{Type::NONE};
  bool hasSuperclass{false};

  void addCode(const OpCode& c, int line) {
    code.push_back(static_cast<uint8_t>(c));
    lines.push_back(line);
  }

  void addOperand(const uint8_t& op) {
    code.push_back(op);
    lines.push_back(0);
  }

  uint8_t addConstant(const Value& v) {
    auto it = std::find(constants.cbegin(), constants.cend(), v);
    if (it == constants.end()) {
      constants.push_back(std::move(v));
      return (uint8_t)(constants.size() - 1);
    } else {
      return (uint8_t)(std::distance(constants.cbegin(), it));
    }
  }
};

}  // namespace compiler
}  // namespace lox