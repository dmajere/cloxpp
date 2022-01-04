#include <gflags/gflags.h>

#include <string>
#include <vector>

#include "chunk.h"
#include "common.h"
#include "compiler.h"
#include "lox.h"
#include "vm.h"

DEFINE_bool(debug, false, "Toggle debug information");

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

  //   lox::lang::Chunk chunk;

  //   auto constant = chunk.addConstant(1.2);
  //   chunk.addCode(lox::lang::OpCode::CONSTANT, 0);
  //   chunk.addOperand(constant);

  //   constant = chunk.addConstant(3.4);
  //   chunk.addCode(lox::lang::OpCode::CONSTANT, 1);
  //   chunk.addOperand(constant);

  //   chunk.addCode(lox::lang::OpCode::ADD, 2);

  //   constant = chunk.addConstant(5.6);
  //   chunk.addCode(lox::lang::OpCode::CONSTANT, 3);
  //   chunk.addOperand(constant);

  //   chunk.addCode(lox::lang::OpCode::DIVIDE, 4);
  //   chunk.addCode(lox::lang::OpCode::NEGATE, 5);

  //   chunk.addCode(lox::lang::OpCode::RETURN, 6);

  //   vm.interpret(chunk);
}