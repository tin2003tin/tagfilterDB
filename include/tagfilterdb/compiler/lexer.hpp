#pragma once

#include "recognizer.hpp"
#include "sourceStream.hpp"
#include "token.hpp"

namespace tagfilterdb::compiler {
class Lexer : public Recognizer, public TokenSource {
  public:
    static constexpr size_t DEFAULT_MODE = 0;
    static constexpr size_t MORE = std::numeric_limits<size_t>::max() - 1;
    static constexpr size_t SKIP = std::numeric_limits<size_t>::max() - 2;

    static constexpr size_t DEFAULT_TOKEN_CHANNEL = Token::DEFAULT_CHANNEL;
    static constexpr size_t HIDDEN = Token::HIDDEN_CHANNEL;
    static constexpr size_t MIN_CHAR_VALUE = 0;
    static constexpr size_t MAX_CHAR_VALUE = 0x10FFFF;

    CharStream *_input;

  protected:
    TokenFactory<CommonToken> *_factory;

  public:
    Unique<Token> token;
    size_t tokenStartCharIndex;
    size_t tokenStartLine;
    size_t tokenStartCharPositionInLine;
    bool hitEOF;
    size_t channel;
    size_t type;
    std::vector<size_t> modeStack;
    size_t mode;
    Lexer();
    Lexer(CharStream *input);
    virtual ~Lexer() {}

    virtual void reset();

    virtual Unique<Token> nextToken() override;

    virtual void skip();
    virtual void more();
    virtual void setMode(size_t m);
    virtual void pushMode(size_t m);
    virtual size_t popMode();

    template <typename T1> void setTokenFactory(TokenFactory<T1> *factory) {
        this->_factory = factory;
    }

    virtual TokenFactory<CommonToken> *getTokenFactory() override;

    virtual void setInputStream(IntStream *input) override;

    virtual std::string getSourceName() override;

    virtual CharStream *getInputStream() override;

    virtual void emit(Unique<Token> newToken);

    virtual Token *emit();

    virtual Token *emitEOF();

    virtual size_t getLine() const override;

    virtual size_t getCharPositionInLine() override;

    virtual void setLine(size_t line);

    virtual void setCharPositionInLine(size_t charPositionInLine);

    virtual size_t getCharIndex();

    virtual std::string getText();

    virtual void setText(const std::string &text);

    virtual Unique<Token> getToken();

    virtual void setToken(Unique<Token> newToken);

    virtual void setType(size_t ttype);

    virtual size_t getType();

    virtual void setChannel(size_t newChannel);

    virtual size_t getChannel();

    virtual const std::vector<std::string> &getChannelNames() const = 0;

    virtual const std::vector<std::string> &getModeNames() const = 0;

    virtual std::vector<Unique<Token>> getAllTokens();

    virtual void recover(const LexerNoViableAltException &e);
    
    virtual void notifyListeners(const LexerNoViableAltException &e);

    virtual std::string getErrorDisplay(const std::string &s);

    virtual void recover(RecognitionException *re);

    virtual size_t getNumberOfSyntaxErrors();

  protected:
    std::string _text;

  private:
    size_t _syntaxErrors;
    void InitializeInstanceFields();
};

} // namespace tagfilterdb::compiler

#include "lexer.cpp"