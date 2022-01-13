#pragma once
#include <vector>

#include "Chunk.h"
#include "Parser.h"
#include "Scope.h"
#include "Value.h"

DECLARE_string(scanner);

namespace lox {
namespace compiler {

class Compiler {
 public:
  Compiler() {}

  Closure compile(const std::string& code) {
    Chunk chunk;
    auto parser = Parser(code, FLAGS_scanner, scope_);
    return parser.run();
  }

 private:
  bool hadError{false};
  Scope scope_;
};

}  // namespace compiler
}  // namespace lox