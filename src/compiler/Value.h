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
  std::string name;
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
      : self(self), method(method) {}
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
  InstanceObject(Class klass) : klass(klass) {}
  Class klass;
  std::unordered_map<std::string, Value> fields;
};

struct StringVisitor {
  std::string operator()(const double d) const { return std::to_string(d); }
  std::string operator()(const bool b) const { return b ? "true" : "false"; }
  std::string operator()(const std::monostate n) const { return "nil"; }
  std::string operator()(const std::string& s) const { return s; }

  std::string operator()(const Function& func) const {
    return "Function<" + func->name() + ">";
  }
  std::string operator()(const Closure& closure) const {
    return "Function<" + closure->function->name() + ">";
  }
  std::string operator()(const NativeFunction& func) const {
    return "Native<" + func->name + ">";
  }
  std::string operator()(const UpvalueValue& upvalue) const {
    if (upvalue->location) {
      return std::visit(StringVisitor(), *upvalue->location);
    }
    return "nil";
  }
  std::string operator()(const Class& klass) const {
    return "Class<" + klass->name + ">";
  }
  std::string operator()(const Instance& instance) const {
    return "Instance<" + instance->klass->name + ">";
  }
  std::string operator()(const BoundMethod& bound) const {
    return "Function<" + bound->method->function->name() + ">";
  }
};

inline std::ostream& operator<<(std::ostream& os, const Value& v) {
  os << std::visit(StringVisitor(), v);
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