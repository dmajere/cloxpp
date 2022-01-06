#pragma once

#include <string>
#include <variant>
#include <vector>

#include "common.h"

namespace lox {
namespace lang {

using Value = std::variant<double, bool, std::monostate, std::string>;

std::ostream& operator<<(std::ostream& os, const Value& v);

struct OutputVisitor {
  void operator()(const double d) const { std::cout << d; }
  void operator()(const bool b) const { std::cout << (b ? "true" : "false"); }
  void operator()(const std::monostate n) const { std::cout << "nil"; }
  void operator()(const std::string& s) const { std::cout << s; }
};

inline std::ostream& operator<<(std::ostream& os, const Value& v) {
  std::visit(OutputVisitor(), v);
  return os;
}

struct FalsinessVisitor {
  bool operator()(const bool b) const { return !b; }
  bool operator()(const std::monostate n) const { return true; }
  template <typename T>
  bool operator()(const T& value) const {
    return false;
  }
};

using Values = std::vector<Value>;

}  // namespace lang
}  // namespace lox