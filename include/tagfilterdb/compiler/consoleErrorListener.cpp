#include "consoleErrorListener.hpp"

using namespace tagfilterdb::compiler;

ConsoleErrorListener ConsoleErrorListener::INSTANCE;

void ConsoleErrorListener::syntaxError(Recognizer * /*recognizer*/,
                                       Token * /*offendingSymbol*/, size_t line,
                                       size_t charPositionInLine,
                                       const std::string &msg,
                                       std::exception_ptr /*e*/) {
    std::cerr << "line " << line << ":" << charPositionInLine << " " << msg
              << std::endl;
}