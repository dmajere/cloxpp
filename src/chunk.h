#pragma once
#include <algorithm>
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
  PRINT,
  POP,
  DEFINE_GLOBAL,
  GET_GLOBAL,
  SET_GLOBAL,
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

}  // namespace lang
}  // namespace lox