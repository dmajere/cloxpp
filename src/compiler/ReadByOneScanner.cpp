#include "ReadByOneScanner.h"

#include "ParseError.h"
#include "Token.h"

namespace lox {
namespace compiler {
ReadByOneScanner::ReadByOneScanner(const std::string& source)
    : Scanner(source), current_token_(end()), previous_token_(end()) {
  advance();
}
ReadByOneScanner::~ReadByOneScanner() {}

const Token& ReadByOneScanner::current() const { return current_token_; }
const Token& ReadByOneScanner::previous() const { return previous_token_; }
const Token& ReadByOneScanner::advance() {
  if (!isEndOfSource()) {
    auto maybeToken = getToken(peekChar());
    while (!maybeToken.has_value()) {
      maybeToken = getToken(peekChar());
    }
    if (maybeToken.value().type == Token::Type::ERROR) {
      parse_error(current(), "Error token after.");
    }

    previous_token_ = std::move(current_token_);
    current_token_ = std::move(maybeToken.value());
  } else {
    previous_token_ = std::move(current_token_);
    current_token_ = end();
  }
  return previous();
}

}  // namespace compiler
}  // namespace lox