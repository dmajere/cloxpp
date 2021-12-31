
#include "chunk.h"
#include "common.h"
#include "debug.h"

int main(int argc, char** argv) {
  lox::lang::Chunk chunk;

  auto offset = chunk.addConstant(1.2);
  chunk.addCode(lox::lang::OpCode::CONSTANT, 0);
  chunk.addOperand(offset);
  chunk.addCode(lox::lang::OpCode::RETURN, 1);

  lox::lang::Disassembler::dis(chunk, "test chunk");
  return 0;
}