#ifndef TAGFILTERDB_INCLUDE_STATUS_HPP_
#define TAGFILTERDB_INCLUDE_STATUS_HPP_

#include <algorithm>
#include <string>
#include <iostream>
#include <cstring>
#include <cassert>

#include "tagfilterdb/export.hpp"

namespace tagfilterdb
{
    class TAGFILTERDB_EXPORT Status
    {
    public:
        Status() noexcept : state_(nullptr) {}
        ~Status() { delete[] state_; }
        Status(const Status &rhs)
        {
            state_ = (rhs.state_ == nullptr) ? nullptr : CopyState(rhs.state_);
        }
        Status &operator=(Status &&rhs) noexcept
        {
            std::swap(state_, rhs.state_);
            return *this;
        }
        Status &operator=(const Status &rhs)
        {
            if (state_ != rhs.state_)
            {
                delete[] state_;
                state_ = (rhs.state_ == nullptr) ? nullptr : CopyState(rhs.state_);
            }
            return *this;
        }
        Status(Status &&rhs) noexcept : state_(rhs.state_) { rhs.state_ = nullptr; }

        static Status OK() { return Status(); }
        static Status NotFound(const std::string &msg, const std::string &msg2 = "")
        {
            return Status(kNotFound, msg, msg2);
        }
        static Status Corruption(const std::string &msg, const std::string &msg2 = "")
        {
            return Status(kCorruption, msg, msg2);
        }
        static Status NotSupported(const std::string &msg, const std::string &msg2 = "")
        {
            return Status(kNotSupported, msg, msg2);
        }
        static Status InvalidArgument(const std::string &msg, const std::string &msg2 = "")
        {
            return Status(kInvalidArgument, msg, msg2);
        }
        static Status IOError(const std::string &msg, const std::string &msg2 = "")
        {
            return Status(kIOError, msg, msg2);
        }
        bool ok() const { return (state_ == nullptr); }

        bool IsNotFound() const { return code() == kNotFound; }

        bool IsCorruption() const { return code() == kCorruption; }

        bool IsIOError() const { return code() == kIOError; }

        bool IsNotSupportedError() const { return code() == kNotSupported; }

        bool IsInvalidArgument() const { return code() == kInvalidArgument; }

        std::string ToString() const;

    private:
        enum Code
        {
            kOk = 0,
            kNotFound = 1,
            kCorruption = 2,
            kNotSupported = 3,
            kInvalidArgument = 4,
            kIOError = 5
        };
        Code code() const
        {
            return (state_ == nullptr) ? kOk : static_cast<Code>(state_[4]);
        }
        const char *state_;
        Status(Code code, const std::string &msg, const std::string &msg2);

        const char *CopyState(const char *state);
    };

}

#endif