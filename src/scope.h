#pragma once
#include <vector>

#include "token.h"
namespace lox {
namespace lang {

struct Local {
  Local(const Token& name, int depth) : name(std::move(name)), depth{depth} {}
  const Token name;
  int depth;
};

class Scope {
 public:
  Scope() : depth_{0} {}
  Scope(int depth) : depth_{depth} {}

  void incrementDepth() { depth_++; }
  int decrementDepth() {
    depth_--;
    size_t old = locals_.size();
    while (!locals_.empty() && locals_.back().depth > depth_) {
      locals_.pop_back();
    }
    return old - locals_.size();
  }

  int depth() const { return depth_; }

  void addLocal(const Token& name) {
    for (auto it = locals_.rbegin(); it != locals_.rend(); it++) {
      if (it->depth != -1 && it->depth < depth_) {
        break;
      }
      if (it->name.lexeme == name.lexeme) {
        throw std::runtime_error("Defined variable");
      }
    }
    locals_.push_back({name, -1});
  }
  void markInitialized() { locals_.back().depth = depth_; }

  int find(const Token& name) {
    auto it = std::find_if(
        locals_.rbegin(), locals_.rend(), [name](const Local& local) -> bool {
          return local.depth != -1 && name.lexeme == local.name.lexeme;
        });

    if (it == locals_.rend()) {
      return -1;
    }
    if (it->depth == -1) {
      throw std::runtime_error("uninitialized variable");
    }
    return locals_.size() - (it - locals_.rbegin()) - 1;
  }
  void debug() const {
    std::cout << "=== Scope ===\nCurrent depth = " << depth_ << "\n";
    for (const auto& l : locals_) {
      std::cout << l.name.lexeme << " depth " << l.depth << "\n";
    }
    std::cout << "=== ===== ===\n";
  }

 private:
  int depth_;
  std::vector<Local> locals_;
};

}  // namespace lang
}  // namespace lox