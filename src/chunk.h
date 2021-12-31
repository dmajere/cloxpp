#pragma once
#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "common.h"
#include "value.h"

namespace lox {
namespace lang {

enum class OpCode {
  CONSTANT,
  RETURN,
};

const std::unordered_map<OpCode, std::string_view> OpCodeToName = {
    {OpCode::RETURN, "RETURN"},
    {OpCode::CONSTANT, "CONSTANT"},
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
    constants.push_back(v);
    return constants.size() - 1;
  }
};

}  // namespace lang
}  // namespace lox