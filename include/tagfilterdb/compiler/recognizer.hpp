#pragma once

#include "atn/ATNDataView.hpp"
#include "proxyErrorListener.hpp"
#include "tagfilterdb/support/casts.hpp"
#include "tagfilterdb/threadManager/synchronization.hpp"

namespace tagfilterdb::compiler {
class Recognizer {
  public:
    static constexpr size_t EOF = std::numeric_limits<size_t>::max();

    Recognizer();
    Recognizer(Recognizer const &) = delete;
    virtual ~Recognizer();

    Recognizer &operator=(Recognizer const &) = delete;

    virtual std::vector<std::string> const &getRuleNames() const = 0;

    virtual Vocabulary const &getVocabulary() const = 0;

    virtual std::map<std::string_view, size_t> getTokenTypeMap();

    virtual std::map<std::string, size_t> getRuleIndexMap();

    virtual size_t getTokenType(std::string_view tokenName);

    virtual atn::ATNDataView getDataView() const {
        throw "there is no serialized ATN";
    }

    virtual std::string getGrammarFileName() const = 0;

    template <class T> T *getInterpreter() const {
        return support::downCast<T *>(_interpreter);
    }

    void setInterpreter(atn::ATNSimulator *interpreter);

    virtual std::string getErrorHeader(RecognitionException *e);

    virtual std::string getTokenErrorDisplay(Token *t);

    virtual void addErrorListener(ErrorListener *listener);

    virtual void removeErrorListener(ErrorListener *listener);

    virtual void removeErrorListeners();

    virtual ProxyErrorListener &getErrorListenerDispatch();

    // virtual bool sempred(RuleContext *localctx, size_t ruleIndex,
    //                      size_t actionIndex);

    // virtual bool precpred(RuleContext *localctx, int precedence);

    // virtual void action(RuleContext *localctx, size_t ruleIndex,
    //                     size_t actionIndex);

    size_t getState() const { return _stateNumber; }

    virtual const atn::ATN &getATN() const = 0;

    void setState(size_t atnState) { _stateNumber = atnState; }

    virtual IntStream *getInputStream() = 0;

    virtual void setInputStream(IntStream *input) = 0;

    virtual TokenFactory<CommonToken> *getTokenFactory() = 0;

    template <typename T1> void setTokenFactory(TokenFactory<T1> *input);

  protected:
    atn::ATNSimulator *_interpreter;
    internal::Mutex _mutex;

  private:
    static std::map<const Vocabulary *, std::map<std::string_view, size_t>>
        _tokenTypeMapCache;
    static std::map<std::vector<std::string>, std::map<std::string, size_t>>
        _ruleIndexMapCache;

    ProxyErrorListener _proxListener; // Manages a collection of listeners.

    size_t _stateNumber;

    void InitializeInstanceFields();
};
} // namespace tagfilterdb::compiler

#include "recognizer.cpp"