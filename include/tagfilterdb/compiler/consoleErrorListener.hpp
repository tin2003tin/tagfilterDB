#pragma once

#include "baseErrorListener.hpp"

namespace tagfilterdb::compiler {
class ConsoleErrorListener : public baseErrorListener {
  public:
    static ConsoleErrorListener INSTANCE;

    virtual void syntaxError(Recognizer *recognizer, Token *offendingSymbol,
                             size_t line, size_t charPositionInLine,
                             const std::string &msg,
                             std::exception_ptr e) override;
};
} // namespace tagfilterdb::compiler

#include "consoleErrorListener.cpp"