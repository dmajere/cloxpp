#pragma once

#include <stdint.h>

#include <iostream>
#include <stack>
#include <string>
#include <unordered_map>
#include <variant>

#include "chunk.h"
#include "compiler.h"
#include "debug.h"
#include "value.h"

DECLARE_bool(debug_stack);

namespace lox {
namespace lang {

class Stack {
 public:
  Value get(size_t i) const { return stack_[i]; }

  const Value& peek() const { return peek(0); }
  const Value& peek(size_t i) const {
    if (stack_.size() < i) {
      throw std::out_of_range("stack out of range");
    }
    return stack_[stack_.size() - i - 1];
  }
  void set(size_t i, const Value& value) { stack_[i] = std::move(value); }

  void pop() { stack_.pop_back(); }
  void push(const Value& value) { stack_.push_back(std::move(value)); }
  void popAndPush(const Value& value) {
    pop();
    push(value);
  }
  void popTwoAndPush(const Value& value) {
    pop();
    pop();
    push(value);
  }
  bool empty() const { return stack_.empty(); }
  size_t size() const { return stack_.size(); }
  auto begin() { return stack_.begin(); }
  auto end() { return stack_.end(); }

 private:
  std::vector<Value> stack_;
};

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
      try {
        auto interpret_result = run();
        return interpret_result;
      } catch (std::runtime_error&) {
        return InterpretResult::RUNTIME_ERROR;
      }
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
  Stack stack_;
  std::unordered_map<std::string, Value> globals_;

  InterpretResult run() {
    for (;;) {
      if (FLAGS_debug_stack) {
        std::cout << "=== Stack ===\n";
        if (!stack_.empty()) {
          for (const auto& v : stack_) {
            std::cout << "=> ";
            Disassembler::value(v);
            std::cout << "\n";
          }
        } else {
          std::cout << "\tempty\n";
        }
        std::cout << "=== ===== ===\n";
      }
      const auto op = read_byte();

#define BINARY_OP(op)                                              \
  do {                                                             \
    binary_op([](double a, double b) -> Value { return a op b; }); \
  } while (false)

      switch (static_cast<OpCode>(op)) {
        case OpCode::RETURN: {
          return InterpretResult::OK;
        }
        case OpCode::PRINT: {
          Disassembler::value(stack_.peek());
          stack_.pop();
          std::cout << "\n";
          break;
        }
        case OpCode::DEFINE_GLOBAL: {
          std::string name = read_string();
          auto it = globals_.find(name);
          if (it != globals_.end()) {
            runtimeError("Variable already defined");
          }
          globals_.insert({std::move(name), std::move(stack_.peek(0))});
          stack_.pop();
          break;
        }
        case OpCode::SET_GLOBAL: {
          auto name = read_string();
          auto it = globals_.find(name);
          if (it == globals_.end()) {
            runtimeError("Undefined variable");
          }

          it->second = std::move(stack_.peek(0));
          break;
        }
        case OpCode::GET_GLOBAL: {
          auto name = read_string();
          auto it = globals_.find(name);
          if (it == globals_.end()) {
            runtimeError("Undefined variable");
          }
          stack_.push(it->second);
          break;
        }
        case OpCode::GET_LOCAL: {
          uint8_t slot = read_byte();
          stack_.push(stack_.get(slot));
          break;
        }
        case OpCode::SET_LOCAL: {
          uint8_t slot = read_byte();
          Value copy = stack_.peek(0);
          stack_.set(slot, copy);
          break;
        }
        case OpCode::POP: {
          stack_.pop();
          break;
        }
        case OpCode::CONSTANT: {
          stack_.push(read_constant());
          break;
        }
        case OpCode::NIL: {
          stack_.push(std::monostate());
          break;
        }
        case OpCode::TRUE: {
          stack_.push(true);
          break;
        }
        case OpCode::FALSE: {
          stack_.push(false);
          break;
        }
        case OpCode::ADD: {
          auto* stack = &stack_;
          auto success = std::visit(
              overloaded{
                  [stack](const double& a, const double& b) -> bool {
                    stack->popTwoAndPush(a + b);
                    return true;
                  },
                  [stack](const std::string& a, const std::string& b) -> bool {
                    stack->popTwoAndPush(a + b);
                    return true;
                  },
                  [stack](auto& a, auto& b) -> bool { return false; },
              },
              stack_.peek(1), stack_.peek(0));
          if (!success) {
            runtimeError("Operands must be two numbers or two strings.");
          }
          break;
        }
        case OpCode::SUBSTRACT: {
          BINARY_OP(-);
          break;
        }
        case OpCode::MULTIPLY: {
          BINARY_OP(*);
          break;
        }
        case OpCode::DIVIDE: {
          BINARY_OP(/);
          break;
        }
        case OpCode::NOT: {
          stack_.popAndPush(isFalsy(stack_.peek()));
          break;
        }
        case OpCode::EQUAL: {
          stack_.popTwoAndPush(stack_.peek(0) == stack_.peek(1));
          break;
        }
        case OpCode::NOT_EQUAL: {
          stack_.popTwoAndPush(stack_.peek(0) != stack_.peek(1));
          break;
        }
        case OpCode::GREATER: {
          BINARY_OP(>);
          break;
        }
        case OpCode::LESS: {
          BINARY_OP(<);
          break;
        }
        case OpCode::GREATER_EQUAL: {
          BINARY_OP(>=);
          break;
        }
        case OpCode::LESS_EQUAL: {
          BINARY_OP(<=);
          break;
        }
        case OpCode::NEGATE: {
          Value value = stack_.peek();
          try {
            auto negated = -std::get<double>(stack_.peek());
            stack_.popAndPush(negated);
          } catch (std::bad_variant_access&) {
            runtimeError("Operand must be a number.");
          }
          break;
        }
        default:
          return InterpretResult::COMPILE_ERROR;
      }
    }
    return InterpretResult::COMPILE_ERROR;
#undef BINARY_OP
  }

  void runtimeError(const std::string& message) {
    int line = ip_ - chunk_.code.end() - 1;
    std::cout << "RuntimeError: " << message << " at line " << line << "\n";
    // resetStack();
    throw std::runtime_error("error");
  }
  inline uint8_t read_byte() { return (*ip_++); }
  inline Value read_constant() { return chunk_.constants[*ip_++]; }
  inline std::string read_string() {
    return std::get<std::string>(read_constant());
  }

  inline void binary_op(std::function<Value(double, double)> op) {
    try {
      auto b = std::get<double>(stack_.peek(0));
      auto a = std::get<double>(stack_.peek(1));

      stack_.popTwoAndPush(op(a, b));
    } catch (std::bad_variant_access&) {
      runtimeError("Operands must be numbers.");
    }
  }

  inline bool isFalsy(const Value& v) {
    return std::visit(FalsinessVisitor(), v);
  }
};

}  // namespace lang
}  // namespace lox
