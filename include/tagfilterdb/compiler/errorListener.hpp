#pragma once

#include "recognitionException.hpp"

namespace tagfilterdb::compiler {
class ErrorListener {
  public:
    virtual ~ErrorListener();
    virtual void syntaxError(Recognizer *recognizer, Token *offendingSymbol,
                             size_t line, size_t charPositionInLine,
                             const std::string &msg, std::exception_ptr e) = 0;
    ;
};
} // namespace tagfilterdb::compiler