#pragma once

#include "compiler/Value.h"

namespace lox {
namespace lang {

class Stack {
 public:
  lox::compiler::Value get(size_t i) const { return stack_[i]; }

  const lox::compiler::Value& peek() const { return peek(0); }
  const lox::compiler::Value& peek(size_t i) const {
    if (stack_.size() < i) {
      throw std::out_of_range("stack out of range");
    }
    return stack_[stack_.size() - i - 1];
  }
  void set(size_t i, const lox::compiler::Value& value) {
    stack_[i] = std::move(value);
  }

  void pop() { stack_.pop_back(); }
  void push(const lox::compiler::Value& value) {
    stack_.push_back(std::move(value));
  }
  void popAndPush(const lox::compiler::Value& value) {
    pop();
    push(value);
  }
  void popTwoAndPush(const lox::compiler::Value& value) {
    pop();
    pop();
    push(value);
  }
  bool empty() const { return stack_.empty(); }
  size_t size() const { return stack_.size(); }
  auto begin() { return stack_.begin(); }
  auto end() { return stack_.end(); }

 private:
  std::vector<lox::compiler::Value> stack_;
};

}  // namespace lang
}  // namespace lox