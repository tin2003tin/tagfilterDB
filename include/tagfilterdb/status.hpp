#ifndef TAGFILTERDB_INCLUDE_STATUS_HPP_
#define TAGFILTERDB_INCLUDE_STATUS_HPP_

#include <algorithm>
#include <string>
#include <cstring>
#include <cassert>

#include "tagfilterdb/export.hpp"
#include <tuple>

// TODO: Optimize memory management to reduce allocations and deallocations for Status objects
// TODO: Implement a function to retrieve the error message without needing to use ToString()
// TODO: Add support for custom error codes and messages
// TODO: Improve thread safety for Status handling in multi-threaded environments
// TODO: Provide additional constructors for initializing Status with different data types
// TODO: Ensure all error codes and messages are properly documented
// TODO: Implement unit tests to verify the correctness of the Status class in various scenarios
// TODO: Add logging functionality for tracking the creation and propagation of errors
// TODO: Handle and log Status object destruction for better debugging and memory management
// TODO: Explore using smart pointers or other mechanisms for better memory safety
// TODO: Consider implementing a move assignment operator for better performance in certain cases

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
        static Status OutOfRange(const std::string &msg, const std::string &msg2 = "")
        {
            return Status(kOutOfRange, msg, msg2);
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

        bool IsError() const { return code() > 0; }

        std::string ToString() const
        {

            if (state_ == nullptr)
            {
                return "OK";
            }
            else
            {
                char tmp[30];
                const char *type;
                switch (code())
                {
                case kOk:
                    type = "OK";
                    break;
                case kNotFound:
                    type = "NotFound: ";
                    break;
                case kCorruption:
                    type = "Corruption: ";
                    break;
                case kNotSupported:
                    type = "Not implemented: ";
                    break;
                case kInvalidArgument:
                    type = "Invalid argument: ";
                    break;
                case kIOError:
                    type = "IO error: ";
                    break;
                case kOutOfRange:
                    type = "Out of range: ";
                    break;

                default:
                    std::snprintf(tmp, sizeof(tmp),
                                  "Unknown code(%d): ", static_cast<int>(code()));
                    type = tmp;
                    break;
                }
                std::string result(type);
                uint32_t length;
                std::memcpy(&length, state_, sizeof(length));
                result.append(state_ + 5, length);
                return result;
            }
        }

    private:
        enum Code
        {
            kOk = 0,
            kNotFound = 1,
            kCorruption = 2,
            kNotSupported = 3,
            kOutOfRange = 4,
            kInvalidArgument = 5,
            kIOError = 6
        };
        Code code() const
        {
            return (state_ == nullptr) ? kOk : static_cast<Code>(state_[4]);
        }
        const char *state_;
        Status(Code code, const std::string &msg, const std::string &msg2)
        {
            assert(code != kOk);
            const uint32_t len1 = static_cast<uint32_t>(msg.size());
            const uint32_t len2 = static_cast<uint32_t>(msg2.size());
            const uint32_t size = len1 + (len2 ? (2 + len2) : 0);
            char *result = new char[size + 5];
            std::memcpy(result, &size, sizeof(size));
            result[4] = static_cast<char>(code);
            std::memcpy(result + 5, msg.data(), len1);
            if (len2)
            {
                result[5 + len1] = ':';
                result[6 + len1] = ' ';
                std::memcpy(result + 7 + len1, msg2.data(), len2);
            }
            state_ = result;
        }

        const char *CopyState(const char *state)
        {
            uint32_t size;
            std::memcpy(&size, state, sizeof(size));
            char *result = new char[size + 5];
            std::memcpy(result, state, size + 5);
            return result;
        }
    };

    template <typename T>
    class OperationResult
    {
    public:
        // Constructors
        OperationResult(const T &value, Status status)
            : value(value), status(status) {}

        OperationResult(T &&value, Status status)
            : value(std::move(value)), status(status) {}
        T value;

        Status status;
    };
}

#endif