#ifndef TAGFILTERDB_INCLUDE_STATUS_HPP_
#define TAGFILTERDB_INCLUDE_STATUS_HPP_

#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>

#include "tagfilterdb/export.hpp"

namespace tagfilterdb {

/// @brief Class representing the status of an operation, including success or
/// various error states.
class TAGFILTERDB_EXPORT Status {
  public:
    /// @brief Enumeration of potential error codes.
    enum Err {
        e_Ok = 0,               ///< Operation was successful.
        e_NotFound = 1,         ///< Resource was not found.
        e_Corruption = 2,       ///< Data corruption was detected.
        e_NotSupported = 3,     ///< Operation is not supported.
        e_OutOfRange = 4,       ///< Value is out of allowed range.
        e_InvalidArgument = 5,  ///< Invalid argument was provided.
        e_IOError = 6,          ///< I/O error occurred.
        e_Timeout = 7,          ///< Operation timed out.
        e_PermissionDenied = 8, ///< Permission denied for the operation.
        e_NetworkError = 9      ///< Network error occurred.
    };

    /// @brief Default constructor for `Status`, initializing to a successful
    /// state.
    Status() noexcept : m_state(nullptr) {}

    /// @brief Destructor for `Status`, cleaning up allocated memory.
    ~Status() { delete[] m_state; }

    /// @brief Copy constructor for `Status`.
    /// @param other The `Status` object to copy from.
    Status(const Status &other) {
        m_state =
            (other.m_state == nullptr) ? nullptr : CopyState(other.m_state);
    }

    /// @brief Move assignment operator for `Status`.
    /// @param other The `Status` object to move from.
    /// @return A reference to the current object.
    Status &operator=(Status &&other) noexcept {
        if (this != &other) {
            delete[] m_state;
            m_state = other.m_state;
            other.m_state = nullptr;
        }
        return *this;
    }

    /// @brief Copy assignment operator for `Status`.
    /// @param other The `Status` object to copy from.
    /// @return A reference to the current object.
    Status &operator=(const Status &other) {
        if (this != &other) {
            delete[] m_state;
            m_state =
                (other.m_state == nullptr) ? nullptr : CopyState(other.m_state);
        }
        return *this;
    }

    /// @brief Equality operator to compare with another `Status` object.
    /// @param other The `Status` object to compare with.
    /// @return `true` if the statuses are equal, otherwise `false`.
    bool operator==(const Status &other) const {
        return ErrorMatch() == other.ErrorMatch();
    }

    /// @brief Equality operator to compare with a specific error code.
    /// @param err The error code to compare with.
    /// @return `true` if the status matches the error code, otherwise `false`.
    bool operator==(Err err) const { return ErrorMatch() == err; }

    /// @brief Move constructor for `Status`.
    /// @param other The `Status` object to move from.
    Status(Status &&other) noexcept : m_state(other.m_state) {
        other.m_state = nullptr;
    }

    /// @brief Factory method for creating an OK `Status`.
    /// @return A `Status` object representing success.
    static Status OK() { return Status(); }

    /// @brief Factory method for creating an error `Status`.
    /// @param e_err The error code.
    /// @param r_msg1 The primary error message.
    /// @param r_msg2 The secondary error message (optional).
    /// @return A `Status` object representing the error.
    static Status Error(Err e_err, const std::string &r_msg1 = "",
                        const std::string &r_msg2 = "") {
        return Status(e_err, r_msg1, r_msg2);
    }

    /// @brief Checks if the status represents success.
    /// @return `true` if the status is OK, otherwise `false`.
    bool ok() const { return (m_state == nullptr); }

    /// @brief Checks if the status represents an error.
    /// @return `true` if the status indicates an error, otherwise `false`.
    bool IsError() const { return ErrorMatch() > e_Ok; }

    /// @brief Converts the status to a human-readable string.
    /// @return A string representing the status.
    std::string ToString() const {
        if (m_state == nullptr) {
            return "OK";
        } else {
            char t_tmp[30];
            const char *type;
            switch (ErrorMatch()) {
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
                std::snprintf(t_tmp, sizeof(t_tmp), "Unknown code(%d): ",
                              static_cast<int>(ErrorMatch()));
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
    /// @brief Internal pointer to the state of the `Status` object.
    const char *m_state;

    /// @brief Private constructor for creating a `Status` with an error and
    /// messages.
    /// @param e_err The error code.
    /// @param r_msg1 The primary error message.
    /// @param r_msg2 The secondary error message.
    Status(Err e_err, const std::string &r_msg1, const std::string &r_msg2) {
        assert(e_err != e_Ok);
        const uint32_t len1 = static_cast<uint32_t>(r_msg1.size());
        const uint32_t len2 = static_cast<uint32_t>(r_msg2.size());
        const uint32_t size = len1 + (len2 ? (2 + len2) : 0);
        char *p_result = new char[size + 5];
        std::memcpy(p_result, &size, sizeof(size));
        p_result[4] = static_cast<char>(e_err);
        std::memcpy(p_result + 5, r_msg1.data(), len1);
        if (len2) {
            p_result[5 + len1] = ':';
            p_result[6 + len1] = ' ';
            std::memcpy(p_result + 7 + len1, r_msg2.data(), len2);
        }
        m_state = p_result;
    }

    /// @brief Copies the state from another `Status` object.
    /// @param p_state The state to copy.
    /// @return A pointer to the copied state.
    const char *CopyState(const char *p_state) {
        uint32_t size;
        std::memcpy(&size, p_state, sizeof(size));
        char *p_result = new char[size + 5];
        std::memcpy(p_result, p_state, size + 5);
        return p_result;
    }

    /// @brief Retrieves the error code from the current state.
    /// @return The error code as an `Err` enum.
    Err ErrorMatch() const {
        return (m_state == nullptr) ? e_Ok : static_cast<Err>(m_state[4]);
    }
};

/// @brief Template class for representing the result of an operation, including
/// a return value and status.
template <typename RETURNTYPE> class OperationResult {
  public:
    /// @brief Constructor for `OperationResult` with a lvalue reference.
    /// @param r_value The return value.
    /// @param a_status The status of the operation.
    OperationResult(const RETURNTYPE &r_value, Status a_status)
        : m_data(r_value), m_status(a_status) {}

    /// @brief Constructor for `OperationResult` with a rvalue reference.
    /// @param r_value The return value (moved).
    /// @param a_status The status of the operation.
    OperationResult(RETURNTYPE &&r_value, Status a_status)
        : m_data(std::move(r_value)), m_status(a_status) {}

    /// @brief The return value of the operation.
    RETURNTYPE m_data;

    /// @brief The status of the operation.
    Status m_status;
};

} // namespace tagfilterdb

#endif
