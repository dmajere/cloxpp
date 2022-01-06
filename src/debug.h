#pragma once
#include <iomanip>
#include <iostream>
#include <string>

#include "chunk.h"

namespace lox {
namespace lang {

class Disassembler {
 public:
  static inline void dis(const Chunk& chunk, const std::string& message) {
    std::cout << "=== " << message << " ===\n";
    dis(chunk);
    std::cout << "=== === ===\n\n";
  }

  static inline void dis(const Chunk& chunk) {
    for (size_t offset = 0; offset < chunk.code.size();) {
      std::cout << std::setfill('0') << std::setw(4) << offset << " ";
      offset = dis(chunk, offset);
      if (offset < 0) {
        break;
      }
    }
  }

  static inline int dis(const Chunk& chunk, size_t offset) {
    if (offset >= chunk.code.size()) {
      std::cout << "Invalid chunk offset\n";
      return -1;
    }
    auto op = static_cast<OpCode>(chunk.code[offset]);
    switch (op) {
      case OpCode::RETURN:
        std::cout << "RETURN";
        break;
      case OpCode::TRUE:
        std::cout << "TRUE";
        break;
      case OpCode::FALSE:
        std::cout << "FALSE";
        break;
      case OpCode::NIL:
        std::cout << "NIL";
        break;
      case OpCode::CONSTANT:
        std::cout << "CONSTANT '";
        value(chunk.constants[chunk.code[++offset]]);
        std::cout << "'";
        break;
      case OpCode::ADD:
        std::cout << "ADD ";
        break;
      case OpCode::SUBSTRACT:
        std::cout << "SUBSTRACT ";
        break;
      case OpCode::MULTIPLY:
        std::cout << "MULTIPLY ";
        break;
      case OpCode::DIVIDE:
        std::cout << "DIVIDE ";
        break;
      case OpCode::NOT:
        std::cout << "NOT ";
        break;
      case OpCode::EQUAL:
        std::cout << "EQUAL ";
        break;
      case OpCode::GREATER_EQUAL:
        std::cout << "GREATER_EQUAL ";
        break;
      case OpCode::LESS_EQUAL:
        std::cout << "LESS_EQUAL ";
        break;
      case OpCode::GREATER:
        std::cout << "GREATER ";
        break;
      case OpCode::LESS:
        std::cout << "LESS ";
        break;
      case OpCode::NOT_EQUAL:
        std::cout << "NOT_EQUAL ";
        break;
      case OpCode::NEGATE:
        std::cout << "NEGATE ";
        break;
      default:
        std::cout << "UNKNOWN " << chunk.code[offset];
        break;
    }
    std::cout << "\n";
    return ++offset;
  }

  static inline void value(const Value& v) { std::cout << v; }
};
}  // namespace lang
}  // namespace lox