#pragma once
#include <folly/Optional.h>

#include <sstream>

#include "ParseError.h"
#include "Token.h"

namespace lox {
namespace compiler {

class Scanner {
 public:
  Scanner(const std::string& source) : source_{source}, current_{0}, line_{1} {}
  Scanner(std::string&& source) : source_{source}, current_{0}, line_{1} {}
  virtual ~Scanner(){};

  virtual const Token& current() const = 0;
  virtual const Token& previous() const = 0;
  virtual const Token& advance() = 0;

  bool isAtEnd() const { return current().type == Token::Type::END; }

  bool check(const Token::Type& type) const {
    return isAtEnd() ? false : current().type == type;
  }

  bool match(const Token::Type& type) {
    if (!check(type)) {
      return false;
    }
    advance();
    return true;
  }

  const Token& consume(const Token::Type& type,
                       const std::string_view& message) {
    if (!check(type)) {
      parse_error(current(), message);
    }
    return advance();
  }

  void synchronize() {
    advance();
    while (!isAtEnd()) {
      if (previous().type == Token::Type::SEMICOLON) return;
      switch (current().type) {
        case Token::Type::CLASS:
        case Token::Type::FOR:
        case Token::Type::FUN:
        case Token::Type::IF:
        case Token::Type::PRINT:
        case Token::Type::RETURN:
        case Token::Type::VAR:
        case Token::Type::WHILE:
          return;
        default:
          advance();
      }
    }
  }

 protected:
  inline bool isEndOfSource() const { return current_ >= source_.length(); }
  inline const char peekChar() const {
    return isEndOfSource() ? 0 : source_[current_];
  }
  inline void nextChar() {
    if (!isEndOfSource()) current_++;
  }
  inline bool matchChar(const char& c) {
    bool result = isEndOfSource() ? false : peekChar() == c;
    if (result) {
      nextChar();
    }
    return result;
  }
  folly::Optional<Token> getToken(const char c) {
    // Single character tokens
    if (matchChar('(')) {
      return left_paren(line_);
    } else if (matchChar(')')) {
      return right_paren(line_);
    } else if (matchChar('{')) {
      return left_brace(line_);
    } else if (matchChar('}')) {
      return right_brace(line_);
    } else if (matchChar(',')) {
      return comma(line_);
    } else if (matchChar('.')) {
      return dot(line_);
    } else if (matchChar('?')) {
      return question(line_);
    } else if (matchChar(':')) {
      return colon(line_);
    } else if (matchChar(';')) {
      return semicolon(line_);
    } else if (matchChar('\n')) {
      line_++;
      return none();
    } else if (matchChar(' ') || matchChar('\t') || matchChar('\r') ||
               matchChar('\0')) {
      return none();  // ignore spaces, tabs and carry

    } else if (matchChar('-')) {  // Single and Double character tokens
      if (matchChar('-')) {
        return minus_minus(line_);
      } else if (matchChar('=')) {
        return minus_equal(line_);
      } else {
        return minus(line_);
      }
    } else if (matchChar('+')) {
      if (matchChar('+')) {
        return plus_plus(line_);
      } else if (matchChar('=')) {
        return plus_equal(line_);
      } else {
        return plus(line_);
      }
    } else if (matchChar('/')) {
      if (matchChar('*')) {
        while (true) {
          if (!peekChar()) {
            return error(line_, "Unterminated multiline comment");
          }
          if (peekChar() != '*') {
            nextChar();
          } else {
            nextChar();
            if (peekChar() == '/') {
              nextChar();
              break;
            }
          }
        }
        return none();
      } else if (matchChar('/')) {
        while (peekChar() && peekChar() != '\n') {
          nextChar();
        }
        return none();
      } else if (matchChar('=')) {
        return slash_equal(line_);
      } else {
        return slash(line_);
      }
    } else if (matchChar('*')) {
      if (matchChar('=')) {
        return star_equal(line_);
      } else {
        return star(line_);
      }
    } else if (matchChar('!')) {
      if (matchChar('=')) {
        return bang_equal(line_);
      } else {
        return bang(line_);
      }
    } else if (matchChar('=')) {
      if (matchChar('=')) {
        return equal_equal(line_);
      } else {
        return equal(line_);
      }
    } else if (matchChar('>')) {
      if (matchChar('=')) {
        return greater_equal(line_);
      } else {
        return greater(line_);
      }
    } else if (matchChar('<')) {
      if (matchChar('=')) {
        return less_equal(line_);
      } else {
        return less(line_);
      }
    } else if (matchChar('"')) {  // string
      return string(line_);
    } else if (isdigit(c)) {  // number
      return number(line_);
    } else if (isalpha(c)) {  // identifiers
      return identifier(line_);
    } else {
      return error(line_, "Unknown charracter: " + std::string{c});
    }
  }
  static inline Token end() { return Token(Token::Type::END, "EOF", -1); }

  static inline void parse_error(const Token& token,
                                 const std::string_view& message) {
    std::stringstream ss;
    ss << "ParseError [line " << token.line << "]: " << message << " [at "
       << token.lexeme << "]\n";
    throw ParseError(ss.str());
  }

 private:
  const std::string source_;
  int current_;
  int line_;

  static inline folly::Optional<Token> none() {
    return folly::Optional<Token>();
  }
  static inline Token error(const int line, const std::string& message) {
    return Token(Token::Type::ERROR, std::move(message), line);
  }
  static inline Token left_paren(const int line) {
    return Token(Token::Type::LEFT_PAREN, "(", line);
  }
  static inline Token right_paren(const int line) {
    return Token(Token::Type::RIGHT_PAREN, ")", line);
  }
  static inline Token left_brace(const int line) {
    return Token(Token::Type::LEFT_BRACE, "{", line);
  }
  static inline Token right_brace(const int line) {
    return Token(Token::Type::RIGHT_BRACE, "}", line);
  }
  static inline Token comma(const int line) {
    return Token(Token::Type::COMMA, ".", line);
  }
  static inline Token dot(const int line) {
    return Token(Token::Type::DOT, ".", line);
  }
  static inline Token question(const int line) {
    return Token(Token::Type::QUESTION, "?", line);
  }
  static inline Token colon(const int line) {
    return Token(Token::Type::COLON, ":", line);
  }
  static inline Token semicolon(const int line) {
    return Token(Token::Type::SEMICOLON, ";", line);
  }
  static inline Token minus_minus(const int line) {
    return Token(Token::Type::MINUS_MINUS, "--", line);
  }
  static inline Token minus_equal(const int line) {
    return Token(Token::Type::MINUS_EQUAL, "-=", line);
  }
  static inline Token minus(const int line) {
    return Token(Token::Type::MINUS, "-", line);
  }
  static inline Token plus_plus(const int line) {
    return Token(Token::Type::PLUS_PLUS, "++", line);
  }
  static inline Token plus_equal(const int line) {
    return Token(Token::Type::PLUS_EQUAL, "+=", line);
  }
  static inline Token plus(const int line) {
    return Token(Token::Type::PLUS, "+", line);
  }
  static inline Token slash_equal(const int line) {
    return Token(Token::Type::SLASH_EQUAL, "/=", line);
  }
  static inline Token slash(const int line) {
    return Token(Token::Type::SLASH, "/", line);
  }
  static inline Token star_equal(const int line) {
    return Token(Token::Type::STAR_EQUAL, "*=", line);
  }
  static inline Token star(const int line) {
    return Token(Token::Type::STAR, "*", line);
  }
  static inline Token bang(const int line) {
    return Token(Token::Type::BANG, "!", line);
  }
  static inline Token bang_equal(const int line) {
    return Token(Token::Type::BANG_EQUAL, "!=", line);
  }
  static inline Token equal_equal(const int line) {
    return Token(Token::Type::EQUAL_EQUAL, "==", line);
  }
  static inline Token equal(const int line) {
    return Token(Token::Type::EQUAL, "=", line);
  }
  static inline Token greater_equal(const int line) {
    return Token(Token::Type::GREATER_EQUAL, ">=", line);
  }
  static inline Token greater(const int line) {
    return Token(Token::Type::GREATER, ">", line);
  }
  static inline Token less_equal(const int line) {
    return Token(Token::Type::LESS_EQUAL, "<=", line);
  }
  static inline Token less(const int line) {
    return Token(Token::Type::LESS, "<", line);
  }
  inline Token string(const int line) {
    int start = current_;
    while (true) {
      if (isEndOfSource()) {
        return error(line, "Unterminated string");
      }
      if (matchChar('\\')) {
        // escape character, for simplification we
        //  only support one char escape characters
        nextChar();
      }
      if (matchChar('"')) {
        break;
      }
      nextChar();
    }
    return Token(Token::Type::STRING,
                 source_.substr(start, current_ - start - 1), line);
  }

  inline Token number(const int line) {
    int start = current_;
    while (isdigit(peekChar())) {
      nextChar();
    }

    if (peekChar() == '.') {
      nextChar();
      if (isdigit(peekChar())) {
        while (isdigit(peekChar())) {
          nextChar();
        }
      } else {
        current_--;
      }
    }
    return Token(Token::Type::NUMBER, source_.substr(start, current_ - start),
                 line);
  }
  inline Token identifier(const int line) {
    int start = current_;
    while (isalnum(peekChar()) || peekChar() == '_') {
      nextChar();
    }
    auto identifier = source_.substr(start, current_ - start);
    Token::Type type;
    try {
      type = kLanguageKeywords.at(identifier);
    } catch (std::out_of_range) {
      type = Token::Type::IDENTIFIER;
    }
    return Token(type, std::move(identifier), line);
  }
};
}  // namespace compiler
}  // namespace lox