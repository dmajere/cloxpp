#pragma once
#include <vector>

#include "chunk.h"
#include "parser.h"
#include "scanner.h"
#include "token.h"
#include "scope.h"

namespace lox {
namespace lang {

class Compiler {
 public:
  Compiler() {}

  bool compile(const std::string& code, Chunk& chunk) {
    Scanner scanner(code);
    auto tokens = scanner.scanTokens();
    auto parser = Parser(tokens, chunk, scope_);
    return parser.parse();
  }

 private:
  bool hadError{false};
  Scope scope_;
};

}  // namespace lang
}  // namespace lox