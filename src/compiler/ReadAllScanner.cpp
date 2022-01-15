#include "ReadAllScanner.h"

namespace lox {
namespace compiler {

ReadAllScanner::ReadAllScanner(const std::string& source)
    : Scanner(std::move(source)), current_token_{0} {
  scan();
}
ReadAllScanner::~ReadAllScanner(){};

const Token& ReadAllScanner::current() const { return tokens_[current_token_]; }
const Token& ReadAllScanner::previous() const {
  return tokens_[current_token_ - 1];
}
const Token& ReadAllScanner::advance() {
  if (!isAtEnd()) current_token_++;
  return previous();
}

void ReadAllScanner::scan() {
  while (const char c = peekChar()) {
    auto maybeToken = getToken(c);
    if (maybeToken.has_value()) {
      if (maybeToken.value().type == Token::Type::ERROR) {
        parse_error(current(), "Unexpected error token");
      }
      tokens_.push_back(std::move(maybeToken.value()));
    }
  }
  tokens_.push_back(end());
}

}  // namespace compiler
}  // namespace lox