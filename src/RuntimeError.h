#pragma once

#include <stdexcept>

namespace lox {
namespace compiler {

struct RuntimeError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

}  // namespace compiler
}  // namespace lox