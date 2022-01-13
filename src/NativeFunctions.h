#pragma once

#include <chrono>
#include <thread>

#include "compiler/Value.h"

namespace lox {
namespace lang {

static lox::compiler::Value clockNative(
    int argCount, std::vector<lox::compiler::Value>::iterator args) {
  return (double)clock() / CLOCKS_PER_SEC;
}

static lox::compiler::Value sleepNative(
    int argCount, std::vector<lox::compiler::Value>::iterator args) {
  try {
    auto duration = static_cast<long long>(std::get<double>(*args));
    std::this_thread::sleep_for(std::chrono::seconds(duration));
    return true;
  } catch (std::bad_variant_access&) {
    return false;
  }
}

}  // namespace lang
}  // namespace lox