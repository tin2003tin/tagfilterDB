#ifndef TAGFILTERDB_INCLUDE_STATUS_HPP_
#define TAGFILTERDB_INCLUDE_STATUS_HPP_

#include <algorithm>
#include <string>
#include <cstring>
#include <cassert>

#include "tagfilterdb/export.hpp"

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
        // Enum for error
        enum Err
        {
            e_Ok = 0,
            e_NotFound = 1,
            e_Corruption = 2,
            e_NotSupported = 3,
            e_OutOfRange = 4,
            e_InvalidArgument = 5,
            e_IOError = 6,
            e_Timeout = 7,
            e_PermissionDenied = 8,
            e_NetworkError = 9
        };

        // Constructors
        Status() noexcept : m_state(nullptr) {}

        ~Status() { delete[] m_state; }

        Status(const Status &other)
        {
            m_state = (other.m_state == nullptr) ? nullptr : CopyState(other.m_state);
        }

        Status &operator=(Status &&other) noexcept
        {
            if (this != &other)
            {
                delete[] m_state;
                m_state = other.m_state;
                other.m_state = nullptr;
            }
            return *this;
        }

        Status &operator=(const Status &other)
        {
            if (this != &other)
            {
                delete[] m_state;
                m_state = (other.m_state == nullptr) ? nullptr : CopyState(other.m_state);
            }
            return *this;
        }

        bool operator==(const Status &other) const
        {
            return ErrorMatch() == other.ErrorMatch();
        }

        bool operator==(Err err) const
        {
            return ErrorMatch() == err;
        }

        Status(Status &&other) noexcept : m_state(other.m_state)
        {
            other.m_state = nullptr;
        }

        // Static factory methods
        static Status OK() { return Status(); }

        static Status Error(Err e_err, const std::string &r_msg1 = "", const std::string &r_msg2 = "")
        {
            return Status(e_err, r_msg1, r_msg2);
        }

        bool ok() const { return (m_state == nullptr); }

        bool IsError() const { return ErrorMatch() > e_Ok; }

        std::string ToString() const
        {
            if (m_state == nullptr)
            {
                return "OK";
            }
            else
            {
                char t_tmp[30];
                const char *type;
                switch (ErrorMatch())
                {
                case e_Ok:
                    type = "OK";
                    break;
                case e_NotFound:
                    type = "NotFound: ";
                    break;
                case e_Corruption:
                    type = "Corruption: ";
                    break;
                case e_NotSupported:
                    type = "Not implemented: ";
                    break;
                case e_InvalidArgument:
                    type = "Invalid argument: ";
                    break;
                case e_IOError:
                    type = "IO error: ";
                    break;
                case e_OutOfRange:
                    type = "Out of range: ";
                    break;
                case e_Timeout:
                    type = "Timeout: ";
                    break;
                case e_PermissionDenied:
                    type = "Permission denied: ";
                    break;
                case e_NetworkError:
                    type = "Network error: ";
                    break;
                default:
                    std::snprintf(t_tmp, sizeof(t_tmp),
                                  "Unknown code(%d): ", static_cast<int>(ErrorMatch()));
                    type = t_tmp;
                    break;
                }
                std::string t_result(type);
                uint32_t length;
                std::memcpy(&length, m_state, sizeof(length));
                t_result.append(m_state + 5, length);
                return t_result;
            }
        }

    private:
        const char *m_state;

        Status(Err e_err, const std::string &r_msg1, const std::string &r_msg2)
        {
            assert(e_err != e_Ok);
            const uint32_t len1 = static_cast<uint32_t>(r_msg1.size());
            const uint32_t len2 = static_cast<uint32_t>(r_msg2.size());
            const uint32_t size = len1 + (len2 ? (2 + len2) : 0);
            char *p_result = new char[size + 5];
            std::memcpy(p_result, &size, sizeof(size));
            p_result[4] = static_cast<char>(e_err);
            std::memcpy(p_result + 5, r_msg1.data(), len1);
            if (len2)
            {
                p_result[5 + len1] = ':';
                p_result[6 + len1] = ' ';
                std::memcpy(p_result + 7 + len1, r_msg2.data(), len2);
            }
            m_state = p_result;
        }

        const char *CopyState(const char *p_state)
        {
            uint32_t size;
            std::memcpy(&size, p_state, sizeof(size));
            char *p_result = new char[size + 5];
            std::memcpy(p_result, p_state, size + 5);
            return p_result;
        }

        Err ErrorMatch() const
        {
            return (m_state == nullptr) ? e_Ok : static_cast<Err>(m_state[4]);
        }
    };

    template <typename RETURNTYPE>
    class OperationResult
    {
    public:
        // Constructors
        OperationResult(const RETURNTYPE &r_value, Status a_status)
            : m_data(r_value), m_status(a_status) {}

        OperationResult(RETURNTYPE &&r_value, Status a_status)
            : m_data(std::move(r_value)), m_status(a_status) {}

        RETURNTYPE m_data;
        Status m_status;
    };
}

#endif
