#pragma once

#include <string>
#include <variant>
#include <vector>

#include "Chunk.h"

namespace lox {
namespace compiler {

struct Chunk;
struct FunctionObject;
struct NativeFunctionObject;
struct ClosureObject;
using Function = std::shared_ptr<FunctionObject>;
using NativeFunction = std::shared_ptr<NativeFunctionObject>;
using Closure = std::shared_ptr<ClosureObject>;

using Value = std::variant<double, bool, std::monostate, std::string, Function,
                           NativeFunction, Closure>;

std::ostream& operator<<(std::ostream& os, const Value& v);

class FunctionObject {
 private:
  const int arity_;
  const std::string name_;
  std::unique_ptr<Chunk> chunk_;

 public:
  FunctionObject(int arity, const std::string& name,
                 std::unique_ptr<Chunk> chunk)
      : arity_{arity}, name_(name), chunk_(std::move(chunk)) {}

  const std::string& name() const { return name_; }
  const int arity() const { return arity_; }
  Chunk& chunk() { return *chunk_; }
};

typedef Value (*NativeFn)(int argCount, std::vector<Value>::iterator args);

struct NativeFunctionObject {
  NativeFn function;
};

class ClosureObject {
 public:
  explicit ClosureObject(Function function) : function(function) {}
  Function function;
};

struct OutputVisitor {
  void operator()(const double d) const { std::cout << d; }
  void operator()(const bool b) const { std::cout << (b ? "true" : "false"); }
  void operator()(const std::monostate n) const { std::cout << "nil"; }
  void operator()(const std::string& s) const {
    std::cout << "\"" << s << "\"";
  }
  void operator()(Function func) const {
    std::cout << "function " << func->name();
  }
  void operator()(Closure closure) const {
    std::cout << "closure " << closure->function->name();
  }
  void operator()(NativeFunction func) const { std::cout << "native function"; }
};

inline std::ostream& operator<<(std::ostream& os, const Value& v) {
  std::visit(OutputVisitor(), v);
  return os;
}

struct FalsinessVisitor {
  bool operator()(const bool b) const { return !b; }
  bool operator()(const std::monostate n) const { return true; }
  template <typename T>
  bool operator()(const T& value) const {
    return false;
  }
};

}  // namespace compiler
}  // namespace lox