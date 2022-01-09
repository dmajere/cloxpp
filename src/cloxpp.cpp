#include <gflags/gflags.h>

#include <string>
#include <vector>

#include "chunk.h"
#include "common.h"
#include "compiler.h"
#include "lox.h"
#include "vm.h"

DEFINE_bool(debug, false, "Toggle debug information");
DEFINE_bool(debug_stack, false, "Toggle debug stack information");

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  std::vector<std::string> arguments(argv, argv + argc);

  auto compiler = std::make_unique<lox::lang::Compiler>();
  auto vm = std::make_unique<lox::lang::VM>(std::move(compiler));
  auto lox = std::make_unique<lox::lang::Lox>(std::move(vm));

  if (arguments.size() == 1) {
    lox->repl();
  } else {
    lox->runFile(arguments[1]);
  }
}