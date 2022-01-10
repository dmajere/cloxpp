#pragma once

#include "Scanner.h"

namespace lox {
namespace compiler {

class ReadByOneScanner : public Scanner {
 public:
  ReadByOneScanner(const std::string& source);
  ~ReadByOneScanner() override;

  const Token& current() const override;
  const Token& previous() const override;
  const Token& advance() override;

 private:
  Token current_token_;
  Token previous_token_;
};
}  // namespace compiler
}  // namespace lox