#pragma once

#include <vector>

#include "Scanner.h"

namespace lox {
namespace compiler {

class ReadAllScanner : public Scanner {
 public:
  ReadAllScanner(const std::string& source);
  ~ReadAllScanner() override;

  const Token& current() const override;
  const Token& previous() const override;
  const Token& advance() override;

 private:
  std::vector<Token> tokens_;
  int current_token_;

  void scan();
};
}  // namespace compiler
}  // namespace lox