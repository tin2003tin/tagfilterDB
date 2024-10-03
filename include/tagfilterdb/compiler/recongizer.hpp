#pragma นืแำ

#include "atn/ATNDataView.hpp"
#include "proxyErrorListener.hpp"
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

    virtual Vocabulary const &getVocubalary() const = 0;

    virtual std::map<std::string_view, size_t> getTokenTypeMap();

    virtual std::map<std::string, size_t> getRuleIndexMap();

    virtual size_t getTokenType(std::string_view tokenName);

    virtual atn::ATNDataView getSerializedATN() const {
        throw "there is no serialized ATN";
    }
    virtual std::string getGrammarFileName() const = 0;
    // getInterpreter()
    // void setInterpreter(atn::ATNSimulator *interpreter);
    //  virtual std::string getErrorHeader(RecognitionException *e);
    virtual std::string getTokenErrorDisplay(Token *t);
    void addErrorListener(ErrorListener *listener);
    void removeErrorListener(ErrorListener *listener);
    void removeErrorListeners();
    virtual ProxyErrorListener &getErrorListenerDispatch();
    // sempred
    // precpred
    // action
    size_t getState() const { return _stateNumber; }

    virtual IntStream *getInputStream() = 0;

    virtual void setInputStream(IntStream *input) = 0;

    virtual TokenFactory<CommonToken> *getTokenFactory() = 0;

    // template <typename T1> void setTokenFactory(TokenFactory<T1> *input);

  protected:
    //    atn::ATNSimulator *_interpreter;
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

#include "recongizer.cpp"