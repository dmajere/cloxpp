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
struct UpvalueObject;
struct ClassObject;
struct InstanceObject;
struct BoundMethodObject;

using Function = std::shared_ptr<FunctionObject>;
using NativeFunction = std::shared_ptr<NativeFunctionObject>;
using Closure = std::shared_ptr<ClosureObject>;
using UpvalueValue = std::shared_ptr<UpvalueObject>;
using Class = std::shared_ptr<ClassObject>;
using Instance = std::shared_ptr<InstanceObject>;
using BoundMethod = std::shared_ptr<BoundMethodObject>;

using Value = std::variant<double, bool, std::monostate, std::string, Function,
                           NativeFunction, Closure, UpvalueValue, Class,
                           Instance, BoundMethod>;

std::ostream& operator<<(std::ostream& os, const Value& v);

class FunctionObject {
 private:
  const int arity_;
  const std::string name_;
  std::unique_ptr<Chunk> chunk_;
  int upvalueCount{0};

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
  std::vector<UpvalueValue> upvalues;
};

class BoundMethodObject {
 public:
  explicit BoundMethodObject(Instance self, Closure method)
      : self(std::move(self)), method(std::move(method)) {}
  Instance self;
  Closure method;
};

struct UpvalueObject {
  Value* location;
  Value closed;
  UpvalueValue next;
  UpvalueObject(Value* slot)
      : location(slot), closed(std::monostate()), next(nullptr) {}
};

struct ClassObject {
  ClassObject(std::string& name) : name(std::move(name)) {}

  std::string name;
  std::unordered_map<std::string, Closure> methods;
};

struct InstanceObject {
  InstanceObject(Class klass) : klass{std::move(klass)} {}
  Class klass;
  std::unordered_map<std::string, Value> fields;
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
    std::cout << "closure <" << closure->function->name() << ">";
  }
  void operator()(NativeFunction func) const { std::cout << "native function"; }
  void operator()(UpvalueValue upvalue) const {
    std::cout << "upvalue " << upvalue->location;
  }
  void operator()(Class klass) const { std::cout << "class " << klass->name; }
  void operator()(Instance instance) const {
    std::cout << "instance " << instance->klass->name;
  }
  void operator()(BoundMethod method) const {
    std::cout << "method " << method->method->function->name() << " in class "
              << method->self->klass->name;
  }
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