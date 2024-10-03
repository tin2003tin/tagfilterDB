#pragma once

#include "tagfilterdb/common.hpp"

namespace tagfilterdb {
class RuntimeException : public std::exception {
  private:
    std::string _message;

  public:
    RuntimeException(const std::string &msg = "")
        : std::exception(), _message(msg) {}

    virtual const char *what() const noexcept override {
        return _message.c_str();
    }
};

class IllegalStateException : public RuntimeException {
  public:
    IllegalStateException(const std::string &msg = "")
        : RuntimeException(msg) {}
    IllegalStateException(IllegalStateException const &) = default;
    ~IllegalStateException() {}
    IllegalStateException &operator=(IllegalStateException const &) = default;
};
class IllegalArgumentException : public RuntimeException {
  public:
    IllegalArgumentException(IllegalArgumentException const &) = default;
    IllegalArgumentException(const std::string &msg = "")
        : RuntimeException(msg) {}
    ~IllegalArgumentException() {}
    IllegalArgumentException &
    operator=(IllegalArgumentException const &) = default;
};

class NullPointerException : public RuntimeException {
  public:
    NullPointerException(const std::string &msg = "") : RuntimeException(msg) {}
    NullPointerException(NullPointerException const &) = default;
    ~NullPointerException() {}
    NullPointerException &operator=(NullPointerException const &) = default;
};

class IndexOutOfBoundsException : public RuntimeException {
  public:
    IndexOutOfBoundsException(const std::string &msg = "")
        : RuntimeException(msg) {}
    IndexOutOfBoundsException(IndexOutOfBoundsException const &) = default;
    ~IndexOutOfBoundsException() {}
    IndexOutOfBoundsException &
    operator=(IndexOutOfBoundsException const &) = default;
};

class UnsupportedOperationException : public RuntimeException {
  public:
    UnsupportedOperationException(const std::string &msg = "")
        : RuntimeException(msg) {}
    UnsupportedOperationException(UnsupportedOperationException const &) =
        default;
    ~UnsupportedOperationException() {}
    UnsupportedOperationException &
    operator=(UnsupportedOperationException const &) = default;
};

class EmptyStackException : public RuntimeException {
  public:
    EmptyStackException(const std::string &msg = "") : RuntimeException(msg) {}
    EmptyStackException(EmptyStackException const &) = default;
    ~EmptyStackException() {}
    EmptyStackException &operator=(EmptyStackException const &) = default;
};

// IOException is not a runtime exception (in the java hierarchy).
// Hence we have to duplicate the RuntimeException implementation.
class IOException : public std::exception {
  private:
    std::string _message;

  public:
    IOException(const std::string &msg = "")
        : std::exception(), _message(msg) {}

    virtual const char *what() const noexcept override {
        return _message.c_str();
    }
};

class CancellationException : public IllegalStateException {
  public:
    CancellationException(const std::string &msg = "")
        : IllegalStateException(msg) {}
    CancellationException(CancellationException const &) = default;
    ~CancellationException() {}
    CancellationException &operator=(CancellationException const &) = default;
};

class ParseCancellationException : public CancellationException {
  public:
    ParseCancellationException(const std::string &msg = "")
        : CancellationException(msg) {}
    ParseCancellationException(ParseCancellationException const &) = default;
    ~ParseCancellationException() {}
    ParseCancellationException &
    operator=(ParseCancellationException const &) = default;
};
} // namespace tagfilterdb