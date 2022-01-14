#pragma once

#include "compiler/Value.h"

constexpr size_t kMaxStackSize{255};

namespace lox {
namespace lang {

class Stack {
 public:
  Stack() { stack_.reserve(kMaxStackSize); }
  lox::compiler::Value& get(size_t i) { return stack_[i]; }

  lox::compiler::Value& back() { return stack_.back(); }
  const lox::compiler::Value& peek() const { return peek(0); }
  const lox::compiler::Value& peek(size_t i) const {
    if (stack_.size() < i) {
      throw std::out_of_range("stack out of range");
    }
    return stack_[stack_.size() - i - 1];
  }
  void set(size_t i, const lox::compiler::Value& value) { stack_[i] = value; }

  void pop() {
    if (!stack_.empty()) stack_.pop_back();
  }
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
  void reset() { stack_.clear(); }
  void resize(size_t size) {
    stack_.resize(size);
    stack_.reserve(kMaxStackSize);
  }

 private:
  std::vector<lox::compiler::Value> stack_;
};

}  // namespace lang
}  // namespace lox