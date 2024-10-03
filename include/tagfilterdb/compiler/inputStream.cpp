#include "inputStream.hpp"

#include <string.h>

#include "intStream.hpp"
#include "tagfilterdb/exceptions.hpp"
#include "tagfilterdb/support/range.hpp"
#include "tagfilterdb/support/utf8.hpp"

using namespace tagfilterdb;
using namespace tagfilterdb::compiler;

InputStream::InputStream() { InitializeInstanceFields(); }

InputStream::InputStream(std::string_view input) : InputStream() {
    load(input.data(), input.length());
}

InputStream::InputStream(const char *data, size_t length) {
    load(data, length);
}

InputStream::InputStream(std::istream &stream) : InputStream() { load(stream); }

void InputStream::load(const std::string &input, bool lenient) {
    load(input.data(), input.size(), lenient);
}

void InputStream::load(const char *data, size_t length, bool lenient) {
    // Remove the UTF-8 BOM if present.
    const char *bom = "\xef\xbb\xbf";
    if (length >= 3 && strncmp(data, bom, 3) == 0) {
        data += 3;
        length -= 3;
    }
    if (lenient) {
        _data = support::Utf8::lenientDecode(std::string_view(data, length));
    } else {
        auto maybe_utf32 =
            support::Utf8::strictDecode(std::string_view(data, length));
        if (!maybe_utf32.has_value()) {
            throw IllegalArgumentException(
                "UTF-8 string contains an illegal byte sequence");
        }
        _data = std::move(maybe_utf32).value();
    }
    p = 0;
}

void InputStream::load(std::istream &stream, bool lenient) {
    if (!stream.good() || stream.eof()) // No fail, bad or EOF.
        return;

    _data.clear();

    std::string s((std::istreambuf_iterator<char>(stream)),
                  std::istreambuf_iterator<char>());
    load(s.data(), s.length(), lenient);
}

void InputStream::reset() { p = 0; }

void InputStream::consume() {
    if (p >= _data.size()) {
        assert(LA(1) == IntStream::EOF);
        throw IllegalStateException("cannot consume EOF");
    }

    if (p < _data.size()) {
        p++;
    }
}

size_t InputStream::LA(ssize_t i) {
    if (i == 0) {
        return 0; // undefined
    }

    ssize_t position = static_cast<ssize_t>(p);
    if (i < 0) {
        i++; // e.g., translate LA(-1) to use offset i=0; then _data[p+0-1]
        if ((position + i - 1) < 0) {
            return IntStream::EOF; // invalid; no char before first char
        }
    }

    if ((position + i - 1) >= static_cast<ssize_t>(_data.size())) {
        return IntStream::EOF;
    }

    return _data[static_cast<size_t>((position + i - 1))];
}

size_t InputStream::LT(ssize_t i) { return LA(i); }

size_t InputStream::index() { return p; }

size_t InputStream::size() { return _data.size(); }

// Mark/release do nothing. We have entire buffer.
ssize_t InputStream::mark() { return -1; }

void InputStream::release(ssize_t /* marker */) {}

void InputStream::seek(size_t index) {
    if (index <= p) {
        p = index; // just jump; don't update stream state (line, ...)
        return;
    }
    // seek forward, consume until p hits index or n (whichever comes first)
    index = std::min(index, _data.size());
    while (p < index) {
        consume();
    }
}

std::string InputStream::getText(const support::Range &range) {
    if (range.start < 0 || range.end < 0) {
        return "";
    }

    size_t start = static_cast<size_t>(range.start);
    size_t stop = static_cast<size_t>(range.end);

    if (stop >= _data.size()) {
        stop = _data.size() - 1;
    }

    size_t count = stop - start + 1;
    if (start >= _data.size()) {
        return "";
    }

    auto maybeUtf8 = support::Utf8::strictEncode(
        std::u32string_view(_data).substr(start, count));
    if (!maybeUtf8.has_value()) {
        throw IllegalArgumentException(
            "Input stream contains invalid Unicode code points");
    }
    return std::move(maybeUtf8).value();
}

std::string InputStream::getSourceName() const {
    if (name.empty()) {
        return IntStream::UNKNOWN_SOURCE_NAME;
    }
    return name;
}

std::string InputStream::toString() const {
    auto maybeUtf8 = support::Utf8::strictEncode(_data);
    if (!maybeUtf8.has_value()) {
        throw IllegalArgumentException(
            "Input stream contains invalid Unicode code points");
    }
    return std::move(maybeUtf8).value();
}

void InputStream::InitializeInstanceFields() { p = 0; }
