#pragma once

#include "charStream.hpp"
#include <string_view>

namespace tagfilterdb::compiler {
class InputStream : public CharStream {
  protected:
    std::u32string _data;
    size_t p;

  public:
    std::string name;
    InputStream();
    InputStream(std::string_view input);
    InputStream(const char *data, size_t length);
    InputStream(std::istream &stream);

    virtual void load(const std::string &input, bool lenient);
    virtual void load(const char *data, size_t length, bool lenient);
    virtual void load(std::istream &stream, bool lenient);

    virtual void load(const std::string &input) { load(input, false); }
    virtual void load(const char *data, size_t length) {
        load(data, length, false);
    }
    virtual void load(std::istream &stream) { load(stream, false); }

    virtual void reset();
    virtual void consume() override;
    virtual size_t LA(ssize_t i) override;
    virtual size_t LT(ssize_t i);
    virtual size_t index() override;
    virtual size_t size() override;
    virtual ssize_t mark() override;
    virtual void release(ssize_t marker) override;
    virtual void seek(size_t index) override;
    virtual std::string getText(const support::Range &range) override;
    virtual std::string getSourceName() const override;
    virtual std::string toString() const override;

  private:
    void InitializeInstanceFields();
};
} // namespace tagfilterdb::compiler

#include "inputStream.cpp"