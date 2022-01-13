#pragma once
#include <folly/Optional.h>

#include <sstream>
#include <vector>

#include "../RuntimeError.h"
#include "Token.h"

namespace lox {
namespace compiler {

struct Local {
  Local(const Token& name, int position)
      : name(name), initialized{false}, position{position}, isCaptured{false} {}

  const Token name;
  bool initialized;
  int position;
  bool isCaptured;
};

class Scope {
 public:
  Scope() { locals_.push_back({}); }

  void declare(const Token& name, int depth) {
    int position;

    auto& scope = locals_.back();
    if (!scope.empty()) {
      position = scope.back().position + 1;
    } else {
      position = 0;
    }

    auto maybeDefined = find(name, depth);
    if (maybeDefined) {
      scope_error(name, "Variable already defined");
    }
    locals_[depth].push_back({name, position});
  }

  void initialize(const Token& name, int depth) {
    auto maybeLocal = find(name, depth);
    if (maybeLocal) {
      maybeLocal->initialized = true;
    }
  }
  void capture(const Token& name) {
    int depth = locals_.size();
    Local* maybeLocal = nullptr;

    while (!maybeLocal) {
      depth--;
      if (depth < 0) {
        break;
      }
      maybeLocal = find(name, depth);
    }

    if (maybeLocal) {
      maybeLocal->isCaptured = true;
      return;
    }

    debug();
    scope_error(name, "unknown variable");
  }

  void push_scope(int depth) {
    while (depth >= locals_.size()) {
      locals_.push_back({});
    }
  }

  size_t pop_scope(int depth) {
    auto size = locals_.back().size();
    locals_.pop_back();
    return size;
  }

  size_t depth() const { return locals_.size() - 1; }

  int find(const Token& name) {
    int depth = locals_.size() - 1;
    auto maybeLocal = find(name, depth);
    auto notinitialized = false;

    while (!maybeLocal) {
      depth--;
      if (depth < 0) {
        break;
      }
      maybeLocal = find(name, depth);
    }

    if (maybeLocal && !maybeLocal->initialized) {
      notinitialized = true;
      maybeLocal = nullptr;
      while (!maybeLocal) {
        depth--;
        if (depth < 0) {
          break;
        }
        maybeLocal = find(name, depth);
      }
    }

    if (maybeLocal) {
      return maybeLocal->position;
    }
    if (notinitialized) {
      scope_error(name, "Unintialized variable.");
    }
    return -1;
  }

  void debug() const {
    std::cout << "Locals depth " << locals_.size();

    for (const auto& scope : locals_) {
      std::cout << "scope ===> " << scope.size() << "\n";
      for (const auto& local : scope) {
        std::cout << ":=> " << local.name.lexeme << "\n";
      }
      std::cout << "<===\n";
    }
  }
  void clear() { locals_.clear(); }
  std::vector<Local>& locals(int depth) { return locals_[depth]; }

 private:
  std::vector<std::vector<Local>> locals_;

  Local* find(const Token& name, int depth) {
    if (depth >= locals_.size()) {
      return nullptr;
    }
    auto& scope = locals_[depth];
    auto it = std::find_if(scope.begin(), scope.end(), [&name](Local& local) {
      return local.name.lexeme == name.lexeme;
    });
    if (it == scope.end()) {
      return nullptr;
    }
    return &(*it);
  }

  void scope_error(const Token& token, const std::string& message) const {
    std::stringstream ss;
    ss << "ScopeError [line " << token.line << "]: " << message << " [at "
       << token.lexeme << "]\n";
    throw RuntimeError(ss.str());
  }
};

}  // namespace compiler
}  // namespace lox