#pragma once

#include "scanner.h"

namespace lox {
namespace lang {

class Compiler {
 public:
  Compiler() {}

  void compile(const std::string& code) {
    Scanner scanner(code);
    auto tokens = scanner.scanTokens();
  }
};

}  // namespace lang
}  // namespace lox