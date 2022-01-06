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
  std::stack<Value> stack_;

  InterpretResult run() {
    for (;;) {
      if (FLAGS_debug) {
        std::cout << "[Stack : ";
        if (!stack_.empty()) {
          Disassembler::value(stack_.top());
        } else {
          std::cout << "empty";
        }
        std::cout << "]\n";
      }
      const auto op = read_byte();

      switch (static_cast<OpCode>(op)) {
        case OpCode::RETURN: {
          std::cout << "Return: ";
          if (!stack_.empty()) {
            Disassembler::value(stack_.top());
          }
          std::cout << "\n";
          return InterpretResult::OK;
        }
        case OpCode::CONSTANT: {
          stack_.push(read_constant());
          break;
        }
        case OpCode::NIL: {
          stack_.push(nil_value());
          break;
        }
        case OpCode::TRUE: {
          stack_.push(boolean_value(true));
          break;
        }
        case OpCode::FALSE: {
          stack_.push(boolean_value(false));
          break;
        }
        case OpCode::ADD: {
          binary_op(ValueType::NUMBER,
                    [](const Value& left, const Value& right) {
                      return number_value(left.as_number() + right.as_number());
                    });
          break;
        }
        case OpCode::SUBSTRACT: {
          binary_op(ValueType::NUMBER,
                    [](const Value& left, const Value& right) {
                      return number_value(left.as_number() - right.as_number());
                    });
          break;
        }
        case OpCode::MULTIPLY: {
          binary_op(ValueType::NUMBER,
                    [](const Value& left, const Value& right) {
                      return number_value(left.as_number() * right.as_number());
                    });
          break;
        }
        case OpCode::DIVIDE: {
          binary_op(ValueType::NUMBER,
                    [](const Value& left, const Value& right) {
                      return number_value(left.as_number() / right.as_number());
                    });
          break;
        }
        case OpCode::NOT: {
          Value value = stack_.top();
          stack_.pop();
          stack_.push(boolean_value(isFalsey(value)));
          break;
        }
        case OpCode::EQUAL: {
          Value right = stack_.top();
          stack_.pop();
          Value left = stack_.top();
          stack_.pop();
          stack_.push(boolean_value(valuesEqual(left, right)));
          break;
        }
        case OpCode::NOT_EQUAL: {
          Value right = stack_.top();
          stack_.pop();
          Value left = stack_.top();
          stack_.pop();
          stack_.push(boolean_value(valuesEqual(left, right)));
          break;
        }
        case OpCode::GREATER: {
          binary_op(
              ValueType::BOOL,
              [](const Value& left, const Value& right) {
                return boolean_value(left.as_bool() > right.as_bool());
              },
              false);
          break;
        }
        case OpCode::LESS: {
          binary_op(
              ValueType::BOOL,
              [](const Value& left, const Value& right) {
                return boolean_value(left.as_bool() < right.as_bool());
              },
              false);
          break;
        }
        case OpCode::GREATER_EQUAL: {
          binary_op(
              ValueType::BOOL,
              [](const Value& left, const Value& right) {
                return boolean_value(left.as_bool() >= right.as_bool());
              },
              false);
          break;
        }
        case OpCode::LESS_EQUAL: {
          binary_op(
              ValueType::BOOL,
              [](const Value& left, const Value& right) {
                return boolean_value(left.as_bool() <= right.as_bool());
              },
              false);
          break;
        }
        case OpCode::NEGATE: {
          Value value = stack_.top();
          if (!value.is_number()) {
            runtimeError("Operand must be a number.");
          }
          stack_.pop();
          stack_.push(number_value(-value.as_number()));
          break;
        }
        default:
          return InterpretResult::COMPILE_ERROR;
      }
    }
    return InterpretResult::COMPILE_ERROR;
  }

  void runtimeError(const std::string& message) {
    int line = ip_ - chunk_.code.end() - 1;
    std::cout << "RuntimeError: " << message << " at line " << line << "\n";
    // resetStack();
    throw std::runtime_error("error");
  }
  inline uint8_t read_byte() { return (*ip_++); }
  inline Value read_constant() { return chunk_.constants[*ip_++]; }

  inline void binary_op(const ValueType& type,
                        std::function<Value(Value, Value)> op,
                        bool run_type_check = true) {
    Value left, right;
    if (run_type_check) {
      check_type(type, left, right);
    }
    stack_.push(op(left, right));
  }

  bool isFalsey(const Value& value) {
    if (value.is_nil()) {
      return true;
    }
    if (value.is_bool()) {
      return !value.as_bool();
    }
    return true;
  }

  bool valuesEqual(const Value& left, const Value& right) {
    if (left.type != right.type) {
      return false;
    }
    switch (left.type) {
      case ValueType::BOOL:
        return left.as_bool() == right.as_bool();
      case ValueType::NIL:
        return true;
      case ValueType::NUMBER:
        return left.as_number() == right.as_number();
      default:
        return false;  // Unreachable
    }
  }

  void check_type(const ValueType& type, Value& left, Value& right) {
    right = stack_.top();
    if (right.type != type) {
      runtimeError("Right operand of binary expression must be a number.");
    }
    stack_.pop();
    left = stack_.top();
    if (left.type != type) {
      stack_.push(right);
      runtimeError("Left operand of binary expression must be a number.");
    }
    stack_.pop();
  }
};

}  // namespace lang
}  // namespace lox
