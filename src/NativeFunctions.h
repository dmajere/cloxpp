#pragma once

#include "compiler/Value.h"

namespace lox {
namespace lang {

static lox::compiler::Value clockNative(
    int argCount, std::vector<lox::compiler::Value>::iterator args) {
  return (double)clock() / CLOCKS_PER_SEC;
}

}  // namespace lang
}  // namespace lox