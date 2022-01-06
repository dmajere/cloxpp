#pragma once

#include <vector>

#include "common.h"

namespace lox {
namespace lang {

enum class ValueType {
  BOOL,
  NIL,
  NUMBER,
};

struct Value {
  ValueType type;
  union {
    bool boolean;
    double number;
  } as;

  bool is_bool() const { return type == ValueType::BOOL; }
  bool is_number() const { return type == ValueType::NUMBER; }
  bool is_nil() const { return type == ValueType::NIL; }

  bool as_bool() const { return as.boolean; }
  double as_number() const { return as.number; }
};

using Values = std::vector<Value>;

static inline Value boolean_value(bool value) {
  return {ValueType::BOOL, .as = {.boolean = value}};
}
static inline Value number_value(double value) {
  return {ValueType::NUMBER, .as = {.number = value}};
}
static inline Value nil_value() { return {ValueType::NIL}; }

}  // namespace lang
}  // namespace lox