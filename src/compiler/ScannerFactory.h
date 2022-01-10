#pragma once
#include "ReadAllScanner.h"
#include "ReadByOneScanner.h"
#include "Scanner.h"

namespace lox {
namespace compiler {
struct ScannerFactory {
  static inline std::function<
      std::unique_ptr<Scanner>(const std::string& source)>
  get(const std::string& scanner) {
    return [&scanner](const std::string& source) -> std::unique_ptr<Scanner> {
      if (scanner == "readall") {
        return std::make_unique<ReadAllScanner>(source);
      }
      if (scanner == "byone") {
        return std::make_unique<ReadByOneScanner>(source);
      }
      return nullptr;
    };
  }
};

}  // namespace compiler
}  // namespace lox