#pragma once

#include <stdint.h>

#include <chrono>
#include <iostream>
#include <stack>
#include <string>
#include <thread>
#include <unordered_map>
#include <variant>

#include "Stack.h"
#include "compiler/Chunk.h"
#include "compiler/Compiler.h"
#include "compiler/Value.h"
#include "compiler/debug.h"

DECLARE_bool(debug);
DECLARE_bool(debug_stack);
#define FRAMES_MAX 64

using namespace lox::compiler;

template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

namespace lox {
namespace lang {

struct CallFrame {
  CallFrame(int ip, unsigned long offset, Function function)
      : ip(ip), stackOffset(offset), function(std::move(function)) {}

  int ip;
  unsigned long stackOffset;
  Function function;
};

class VM {
 public:
  enum class InterpretResult { OK, COMPILE_ERROR, RUNTIME_ERROR };

  VM(std::unique_ptr<Compiler> compiler) : compiler_(std::move(compiler)) {}
  ~VM() = default;

  InterpretResult interpret(const std::string& code) {
    lox::compiler::Function func = compiler_->compile(code);
    if (func) {
      try {
        stack_.push(func);
        call(func, 0);
        auto interpret_result = run(func->chunk());
        return interpret_result;
      } catch (std::runtime_error&) {
        return InterpretResult::RUNTIME_ERROR;
      }
    } else {
      return InterpretResult::COMPILE_ERROR;
    }
  }

  InterpretResult interpret(lox::compiler::Chunk& chunk) { return run(chunk); }

  bool call(const Function& function, int argCount) {
    if (argCount != function->arity()) {
      runtimeError("Function arity mismatch");
      return false;
    }

    if (frames_.size() + 1 == FRAMES_MAX) {
      runtimeError("Stack overflow.");
      return false;
    }

    if (FLAGS_debug) {
      Disassembler::dis(function->chunk(), function->name());
    }

    unsigned long offset = stack_.size() - argCount - 1;
    frames_.emplace_back(CallFrame(0, offset, function));
    return true;
  }

  void runtimeError(const std::string& message) {
    // int line = ip_ - chunk_.code.end() - 1;
    std::cout << "RuntimeError: " << message << "\n";
    // resetStack();
    throw std::runtime_error("error");
  }

 private:
  std::unique_ptr<Compiler> compiler_;
  Stack stack_;
  std::unordered_map<std::string, Value> globals_;
  std::vector<CallFrame> frames_;

  struct CallVisitor {
    const int argCount;
    VM* vm;

    CallVisitor(int argCount, VM* vm) : argCount(argCount), vm(vm) {}

    bool operator()(const Function& func) const {
      return vm->call(func, argCount);
    }

    template <typename T>
    bool operator()(const T& value) const {
      vm->runtimeError("Can only call functions and classes.");
      return false;
    }
  };
  bool callValue(const Value& callee, int argCount) {
    return std::visit(CallVisitor(argCount, this), callee);
  }

  InterpretResult run(Chunk& chunk) {
    auto read_byte = [this]() -> uint8_t {
      return this->frames_.back().function->chunk().code.at(
          this->frames_.back().ip++);
    };
    auto read_short = [this]() -> uint16_t {
      this->frames_.back().ip += 2;
      uint8_t left = this->frames_.back().function->chunk().code.at(
          this->frames_.back().ip - 2);
      uint8_t right = this->frames_.back().function->chunk().code.at(
          this->frames_.back().ip - 1);
      return (uint16_t)(left << 8 | right);
    };
    auto read_constant = [this]() -> Value {
      return this->frames_.back()
          .function->chunk()
          .constants[this->frames_.back().function->chunk().code.at(
              this->frames_.back().ip++)];
    };
    auto read_string = [&read_constant]() -> std::string {
      return std::get<std::string>(read_constant());
    };

    for (;;) {
      const uint8_t op = read_byte();

      if (FLAGS_debug_stack) {
        std::cout << "=== Stack: " << static_cast<int>(op) << " ===\n";
        std::cout << "Stack Size " << stack_.size() << std::endl;
        if (!stack_.empty()) {
          for (const auto& v : stack_) {
            std::cout << "=> ";
            lox::compiler::Disassembler::value(v);
            std::cout << "\n";
          }
        } else {
          std::cout << "\tempty\n";
        }
        std::cout << "=== ===== ===\n";
      }

#define BINARY_OP(op)                                              \
  do {                                                             \
    binary_op([](double a, double b) -> Value { return a op b; }); \
  } while (false)

      switch (static_cast<OpCode>(op)) {
        case OpCode::CALL: {
          int argCount = read_byte();
          if (!callValue(stack_.peek(argCount), argCount)) {
            return InterpretResult::RUNTIME_ERROR;
          }
          break;
        }
        case OpCode::LOOP: {
          uint16_t offset = read_short();
          this->frames_.back().ip -= offset;
          break;
        }
        case OpCode::JUMP_IF_FALSE: {
          uint16_t offset = read_short();
          if (isFalsy(stack_.peek(0))) {
            this->frames_.back().ip += offset;
          }
          break;
        }
        case OpCode::JUMP: {
          uint16_t offset = read_short();
          this->frames_.back().ip += offset;
          break;
        }
        case OpCode::RETURN: {
          auto returnValue = stack_.peek();
          stack_.pop();

          auto lastOffset = frames_.back().stackOffset;
          while (stack_.size() != lastOffset) {
            stack_.pop();
          }

          frames_.pop_back();

          if (frames_.empty()) {
            stack_.pop();
            return InterpretResult::OK;
          }

          break;
        }
        case OpCode::PRINT: {
          std::cout << "[Out]: ";
          lox::compiler::Disassembler::value(stack_.peek());
          std::cout << "\n";
          // std::this_thread::sleep_for(std::chrono::seconds(5));
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
          stack_.push(stack_.get(frames_.back().stackOffset + slot));
          break;
        }
        case OpCode::SET_LOCAL: {
          uint8_t slot = read_byte();
          Value copy = stack_.peek(0);
          stack_.set(frames_.back().stackOffset + slot, copy);
          break;
        }
        case OpCode::POP: {
          if (!stack_.empty()) {
            stack_.pop();
          }
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
                  [stack](const std::string& a, const double& b) -> bool {
                    stack->popTwoAndPush(a + std::to_string(b));
                    return true;
                  },
                  [stack](const double& a, const std::string& b) -> bool {
                    stack->popTwoAndPush(std::to_string(a) + b);
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
