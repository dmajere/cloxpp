#pragma once

#include <iostream>
#include <string>
#include <unordered_map>

namespace lox {
namespace compiler {

struct Token {
  enum class Type {
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

  Token(const Token::Type type, const std::string& lexeme, const int line)
      : type(type), lexeme(lexeme), line{line} {};
  Token(const Token& other)
      : type{other.type}, lexeme(other.lexeme), line{other.line} {}
  Token(Token&& other)
      : type{other.type}, lexeme(std::move(other.lexeme)), line{other.line} {}
  ~Token() = default;
  lox::compiler::Token& operator=(lox::compiler::Token&& other) {
    type = other.type;
    lexeme = std::move(other.lexeme);
    line = other.line;
    return *this;
  }

  Token::Type type;
  std::string lexeme;
  int line;
};

const std::unordered_map<std::string, Token::Type> kLanguageKeywords = {
    {"and", Token::Type::AND},       {"class", Token::Type::CLASS},
    {"else", Token::Type::ELSE},     {"false", Token::Type::FALSE},
    {"fun", Token::Type::FUN},       {"for", Token::Type::FOR},
    {"if", Token::Type::IF},         {"nil", Token::Type::NIL},
    {"or", Token::Type::OR},         {"print", Token::Type::PRINT},
    {"return", Token::Type::RETURN}, {"super", Token::Type::SUPER},
    {"this", Token::Type::THIS},     {"true", Token::Type::TRUE},
    {"var", Token::Type::VAR},       {"while", Token::Type::WHILE},
    {"break", Token::Type::BREAK},   {"continue", Token::Type::CONTINUE},
    {"lambda", Token::Type::LAMBDA},
};

}  // namespace compiler
}  // namespace lox