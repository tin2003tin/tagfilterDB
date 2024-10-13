#pragma once

#include "errorListener.hpp"
#include "tagfilterdb/exceptions.hpp"

namespace tagfilterdb::compiler {
class ProxyErrorListener : public ErrorListener {
  private:
    std::set<ErrorListener *> _delegates;

  public:
    void addErrorListener(ErrorListener *listener) {
        if (listener == nullptr) {
            throw "listener cannot be null.";
        }

        _delegates.insert(listener);
    }
    void removeErrorListener(ErrorListener *listener) {
        _delegates.erase(listener);
    }
    void removeErrorListeners() { _delegates.clear(); }

    void syntaxError(Recognizer *recognizer, Token *offendingSymbol,
                     size_t line, size_t charPositionInLine,
                     const std::string &msg, std::exception_ptr e) override {
        for (auto *listener : _delegates) {
            listener->syntaxError(recognizer, offendingSymbol, line,
                                  charPositionInLine, msg, e);
        }
    }
};
} // namespace tagfilterdb::compiler