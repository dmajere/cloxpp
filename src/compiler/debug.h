#pragma once
#include <iomanip>
#include <iostream>
#include <string>

#include "Chunk.h"
#include "Value.h"

namespace lox {
namespace compiler {

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
      case OpCode::LOOP: {
        uint16_t jump_ =
            (uint16_t)((chunk.code[++offset] << 8) | chunk.code[++offset]);
        std::cout << "LOOP " << jump_;
        break;
      }
      case OpCode::JUMP_IF_FALSE: {
        uint16_t jump_ =
            (uint16_t)((chunk.code[++offset] << 8) | chunk.code[++offset]);
        std::cout << "JUMP_IF_FALSE " << jump_;
        break;
      }
      case OpCode::JUMP: {
        uint16_t jump_ =
            (uint16_t)((chunk.code[++offset] << 8) | chunk.code[++offset]);
        std::cout << "JUMP " << jump_;
        offset++;
        break;
      }
      case OpCode::CALL:
        std::cout << "CALL";
        offset++;
        break;
      case OpCode::CLOSURE:
        std::cout << "CLOSURE ";
        value(chunk.constants[chunk.code[++offset]]);
        break;
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
      case OpCode::DEFINE_GLOBAL:
        std::cout << "DEFINE_GLOBAL '";
        value(chunk.constants[chunk.code[++offset]]);
        std::cout << "'";
        break;
      case OpCode::GET_UPVALUE:
        std::cout << "GET_UPVALUE '";
        offset++;
        break;
      case OpCode::SET_UPVALUE: {
        std::cout << "SET_UPVALUE '";
        offset++;
        break;
      }
      case OpCode::GET_GLOBAL:
        std::cout << "GET_GLOBAL '";
        value(chunk.constants[chunk.code[++offset]]);
        std::cout << "'";
        break;
      case OpCode::SET_GLOBAL:
        std::cout << "SET_GLOBAL '";
        value(chunk.constants[chunk.code[++offset]]);
        std::cout << "'";
        break;
      case OpCode::GET_LOCAL:
        std::cout << "GET_LOCAL '" << chunk.code[++offset] << "'";
        break;
      case OpCode::SET_LOCAL:
        std::cout << "SET_LOCAL '" << chunk.code[++offset] << "'";
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
      case OpCode::PRINT:
        std::cout << "PRINT ";
        break;
      case OpCode::POP:
        std::cout << "POP ";
        break;
      default:
        std::cout << "UNKNOWN " << chunk.code[offset];
        break;
    }
    // std::cout << "\n";
    std::cout << std::endl;
    return ++offset;
  }

  static inline void value(const Value& v) { std::cout << v; }
};
}  // namespace compiler
}  // namespace lox