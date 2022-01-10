#include <gflags/gflags.h>

#include <string>
#include <vector>

#include "common.h"
#include "compiler/Compiler.h"
#include "lox.h"
#include "vm.h"

DEFINE_bool(debug, false, "Toggle debug information");
DEFINE_bool(debug_stack, false, "Toggle debug stack information");
DEFINE_string(scanner, "readall", "Scanner type [readall | byone]");

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  std::vector<std::string> arguments(argv, argv + argc);

  auto compiler = std::make_unique<lox::compiler::Compiler>();
  auto vm = std::make_unique<lox::lang::VM>(std::move(compiler));
  auto lox = std::make_unique<lox::lang::Lox>(std::move(vm));

  if (arguments.size() == 1) {
    lox->repl();
  } else {
    lox->runFile(arguments[1]);
  }
}