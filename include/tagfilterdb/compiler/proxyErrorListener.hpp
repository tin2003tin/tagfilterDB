#pragma once

#include "errorListener.hpp"
#include "tagfilterdb/exceptions.hpp"

namespace tagfilterdb::compiler {
class ProxyErrorListener : public ErrorListener {
  private:
    std::set<ErrorListener *> delegates;

  public:
    void addErrorListener(ErrorListener *listener);
    void removeErrorListener(ErrorListener *listener);
    void removeErrorListeners();

    void syntaxError(Recognizer *recognizer, Token *offendingSymbol,
                     size_t line, size_t charPositionInLine,
                     const std::string &msg, std::exception_ptr e) override;
};
} // namespace tagfilterdb::compiler