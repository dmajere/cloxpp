#pragma once
#include <vector>

#include "Parser.h"
#include "Value.h"

DECLARE_string(scanner);

namespace lox {
namespace compiler {

class Compiler {
 public:
  Compiler() {}

  Closure compile(const std::string& code) {
    auto parser = Parser(code, FLAGS_scanner);
    return parser.run();
  }

 private:
  bool hadError{false};
};

}  // namespace compiler
}  // namespace lox