#pragma once

#include <stdint.h>

#include <iostream>
#include <stack>
#include <string>

#include "chunk.h"
#include "compiler.h"
#include "debug.h"
#include "value.h"

DECLARE_bool(debug);

namespace lox {
namespace lang {

constexpr size_t STACK_SIZE_LIMIT{256};

class VM {
 public:
  enum class InterpretResult { OK, COMPILE_ERROR, RUNTIME_ERROR };
  VM(std::unique_ptr<Compiler> compiler) : compiler_(std::move(compiler)) {}
  ~VM() = default;

  InterpretResult interpret(const std::string& code) {
    Chunk chunk;
    if (compiler_->compile(code, chunk)) {
      chunk_ = std::move(chunk);
      ip_ = chunk_.code.begin();
      auto interpret_result = run();
      return interpret_result;
    } else {
      return InterpretResult::COMPILE_ERROR;
    }
  }

  InterpretResult interpret(const Chunk& chunk) {
    this->chunk_ = std::move(chunk);
    this->ip_ = chunk_.code.begin();
    return run();
  }

 private:
  std::unique_ptr<Compiler> compiler_;
  Chunk chunk_;
  std::vector<uint8_t>::iterator ip_;
  std::stack<Value> stack_;

  InterpretResult run() {
    for (;;) {
      if (FLAGS_debug) {
        if (!stack_.empty()) {
          std::cout << "[Stack top: " << stack_.top() << "]\n";
        }
      }
      const auto op = read_byte();

      switch (static_cast<OpCode>(op)) {
        case OpCode::RETURN: {
          std::cout << "Return: ";
          if (!stack_.empty()) {
            std::cout << stack_.top();
          }
          std::cout << "\n";
          return InterpretResult::OK;
        }
        case OpCode::CONSTANT: {
          stack_.push(read_constant());
          break;
        }
        case OpCode::ADD: {
          binary_op([](Value left, Value right) { return left + right; });
          break;
        }
        case OpCode::SUBSTRACT: {
          binary_op([](Value left, Value right) { return left - right; });
          break;
        }
        case OpCode::MULTIPLY: {
          binary_op([](Value left, Value right) { return left * right; });
          break;
        }
        case OpCode::DIVIDE: {
          binary_op([](Value left, Value right) { return left / right; });
          break;
        }
        case OpCode::NEGATE: {
          Value value = stack_.top();
          stack_.pop();
          stack_.push(std::move(-value));
          break;
        }
        default:
          return InterpretResult::COMPILE_ERROR;
      }
    }
    return InterpretResult::COMPILE_ERROR;
  }

  inline uint8_t read_byte() { return (*ip_++); }
  inline Value read_constant() { return chunk_.constants[*ip_++]; }
  inline void binary_op(std::function<Value(Value, Value)> op) {
    Value right = stack_.top();
    stack_.pop();
    Value left = stack_.top();
    stack_.pop();
    stack_.push(op(left, right));
  }
};

}  // namespace lang
}  // namespace lox
