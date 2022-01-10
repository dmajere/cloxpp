#pragma once

#include <stdexcept>

namespace lox {
namespace compiler {

struct ParseError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

}  // namespace compiler
}  // namespace lang