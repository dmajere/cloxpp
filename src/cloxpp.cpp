#include <gflags/gflags.h>

#include "chunk.h"
#include "common.h"
#include "vm.h"

DEFINE_bool(debug, false, "Toggle debug information");

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  lox::lang::VM vm;
  lox::lang::Chunk chunk;

  auto constant = chunk.addConstant(1.2);
  chunk.addCode(lox::lang::OpCode::CONSTANT, 0);
  chunk.addOperand(constant);

  constant = chunk.addConstant(3.4);
  chunk.addCode(lox::lang::OpCode::CONSTANT, 1);
  chunk.addOperand(constant);

  chunk.addCode(lox::lang::OpCode::ADD, 2);

  constant = chunk.addConstant(5.6);
  chunk.addCode(lox::lang::OpCode::CONSTANT, 3);
  chunk.addOperand(constant);

  chunk.addCode(lox::lang::OpCode::DIVIDE, 4);
  chunk.addCode(lox::lang::OpCode::NEGATE, 5);

  chunk.addCode(lox::lang::OpCode::RETURN, 6);

  vm.interpret(chunk);
  return 0;
}