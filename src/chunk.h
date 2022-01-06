#pragma once
#include <cstdint>
#include <iostream>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "common.h"
#include "value.h"

namespace lox {
namespace lang {

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
};

struct Chunk {
  std::vector<uint8_t> code;
  std::vector<int> lines;
  Values constants;

  void addCode(const OpCode& c, int line) {
    code.push_back(static_cast<uint8_t>(c));
    lines.push_back(line);
  }
  void addOperand(const uint8_t& op) {
    code.push_back(op);
    lines.push_back(0);
  }

  int addConstant(const Value& v) {
    constants.push_back(std::move(v));
    return constants.size() - 1;
  }
};

}  // namespace lang
}  // namespace lox