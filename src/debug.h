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
      case OpCode::CONSTANT:
        std::cout << "CONSTANT '" << chunk.constants[chunk.code[++offset]]
                  << "'";
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
};
}  // namespace lang
}  // namespace lox