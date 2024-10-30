#include "charStream.hpp"
#include "commonToken.hpp"
#include "tagfilterdb/support/rangeSet.hpp"

#include "commontokenFactory.hpp"

using namespace tagfilterdb;
using namespace tagfilterdb::compiler;

const Unique<TokenFactory<CommonToken>>
    CommonTokenFactory::DEFAULT(new CommonTokenFactory);

CommonTokenFactory::CommonTokenFactory(bool copyText_) : copyText(copyText_) {}

CommonTokenFactory::CommonTokenFactory() : CommonTokenFactory(false) {}

Unique<CommonToken> CommonTokenFactory::create(SourceStream source, size_t type,
                                               const std::string &text,
                                               size_t channel, size_t start,
                                               size_t stop, size_t line,
                                               size_t charPositionInLine) {
    Unique<CommonToken> token(
        new CommonToken(source, type, channel, start, stop));
    token->setLine(line);
    token->setCharPositionInLine(charPositionInLine);
    if (text != "") {
        token->setText(text);
    } else if (copyText && source.charStream != nullptr) {
        token->setText(source.charStream->getText(support::Range(start, stop)));
    }
    return token;
}

Unique<CommonToken> CommonTokenFactory::create(size_t type,
                                               const std::string &text) {
    return Unique<CommonToken>(new CommonToken(type, text));
}