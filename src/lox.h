#pragma once
#include <folly/File.h>
#include <folly/FileUtil.h>
#include <unistd.h>

#include <iostream>
#include <string_view>

#include "vm.h"

constexpr std::string_view kLoxInputPrompt{"[In]: "};
constexpr std::string_view kLoxOutputPrompt{"[Out]: "};

namespace lox {
namespace lang {
class Lox {
 public:
  Lox(std::unique_ptr<VM> vm) : vm_(std::move(vm)) {}

  void exit(const VM::InterpretResult& result) {
    switch (result) {
      case VM::InterpretResult::COMPILE_ERROR:
        std::exit(65);
      case VM::InterpretResult::RUNTIME_ERROR:
        std::exit(70);
      case VM::InterpretResult::OK:
        std::exit(0);
    }
  }

  void repl() {
    for (std::string line;; std::getline(std::cin, line)) {
      if (std::cin.fail()) {
        return;
      }
      if (!line.empty()) {
        vm_->interpret(line);
      }
      std::cout << kLoxInputPrompt;
    }
  }
  void runFile(const std::string& path) {
    auto f = folly::File(path);
    std::string code;
    folly::readFile(f.fd(), code);
    auto result = vm_->interpret(code);
    this->exit(result);
  }

 private:
  std::unique_ptr<VM> vm_;
};
}  // namespace lang
}  // namespace lox