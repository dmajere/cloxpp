#pragma once

#include <iostream>
#include <string>

namespace lox {
namespace lang {

struct Token {
  enum class TokenType {
    // single character
    LEFT_PAREN,
    RIGHT_PAREN,
    LEFT_BRACE,
    RIGHT_BRACE,
    COMMA,
    DOT,
    MINUS,
    PLUS,
    COLON,
    SEMICOLON,
    SLASH,
    STAR,
    BANG,
    EQUAL,
    GREATER,
    LESS,
    QUESTION,

    // double character
    BANG_EQUAL,
    EQUAL_EQUAL,
    GREATER_EQUAL,
    LESS_EQUAL,
    MINUS_EQUAL,
    PLUS_EQUAL,
    SLASH_EQUAL,
    STAR_EQUAL,
    MINUS_MINUS,
    PLUS_PLUS,

    // literals
    IDENTIFIER,
    STRING,
    NUMBER,

    // keywords
    AND,
    CLASS,
    ELSE,
    FALSE,
    FUN,
    LAMBDA,
    FOR,
    IF,
    NIL,
    OR,
    PRINT,
    RETURN,
    SUPER,
    THIS,
    TRUE,
    VAR,
    WHILE,
    BREAK,
    CONTINUE,
    END,
    ERROR,
  };

  Token(const TokenType type, const std::string& lexeme, const int line)
      : type(type), lexeme(lexeme), line(line){};
  ~Token() = default;

  const TokenType type;
  const std::string lexeme;
  const int line;
};

}  // namespace lang
}  // namespace lox