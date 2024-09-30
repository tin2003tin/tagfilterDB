#pragma once

#include "writableToken.hpp"

namespace tagfilterdb::compiler {
class CommonToken : public WritableToken {
    static const SourceStream EMPTY_SOURCE;
    size_t _type;
    size_t _line;
    size_t _charPositionInLine;
    size_t _channel;
    SourceStream _source;
    std::string _text;
    size_t _index;
    size_t _start;
    size_t _stop;

  public:
    CommonToken(size_t type);
    CommonToken(SourceStream source, size_t type, size_t channel, size_t start,
                size_t stop);
    CommonToken(size_t type, const std::string &text);
    CommonToken(Token *oldToken);

    virtual size_t getType() const override;
    virtual void setType(size_t type) override;

    virtual void setText(const std::string &text) override;
    virtual std::string getText() const override;

    virtual void setLine(size_t line) override;
    virtual size_t getLine() const override;

    virtual size_t getCharPositionInLine() const override;
    virtual void setCharPositionInLine(size_t charPositionInLine) override;

    virtual size_t getChannel() const override;
    virtual void setChannel(size_t channel) override;

    virtual size_t getStartIndex() const override;
    virtual void setStartIndex(size_t start);

    virtual size_t getStopIndex() const override;
    virtual void setStopIndex(size_t stop);

    virtual size_t getTokenIndex() const override;
    virtual void setTokenIndex(size_t index) override;

    virtual TokenSource *getTokenSource() const override;
    virtual CharStream *getInputStream() const override;

    virtual std::string toString() const override;

    virtual std::string toString(Recognizer *r) const;

  private:
    void InitializeInstanceFields();
};
} // namespace tagfilterdb::compiler

#include "commonToken.cpp"