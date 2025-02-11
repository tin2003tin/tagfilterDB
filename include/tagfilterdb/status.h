#ifndef TAGFILTERDB_STATUS_H
#define TAGFILTERDB_STATUS_H

#include <cstdio>
#include <string>
#include <memory>
#include <cassert>
#include <cstring>

namespace tagfilterdb {
    class Status {
        public:

        Status() noexcept : state_(nullptr) {}
        ~Status() { delete[] state_; }
        static Status OK() { return Status(); }

        static Status NotFound(const std::string& msg, const std::string& msg2 = std::string()) {
        return Status(kNotFound, msg, msg2);
        }
        static Status Corruption(const std::string& msg, const std::string& msg2 = std::string()) {
        return Status(kCorruption, msg, msg2);
        }
        static Status NotSupported(const std::string& msg, const std::string& msg2 = std::string()) {
        return Status(kNotSupported, msg, msg2);
        }
        static Status InvalidArgument(const std::string& msg, const std::string& msg2 = std::string()) {
        return Status(kInvalidArgument, msg, msg2);
        }
        static Status IOError(const std::string& msg, const std::string& msg2 = std::string()) {
        return Status(kIOError, msg, msg2);
        }

        bool ok() const { return (state_ == nullptr); }
        bool IsNotFound() const { return code() == kNotFound; }
        bool IsIOError() const { return code() == kIOError; }

        std::string ToString() const {
          if (state_ == nullptr) {
            return "OK";
          } else {
            char tmp[30];
            const char* type;
            switch (code()) {
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
        enum Code {
            kOk = 0,
            kNotFound = 1,
            kCorruption = 2,
            kNotSupported = 3,
            kInvalidArgument = 4,
            kIOError = 5
          };

          Code code() const {
            return (state_ == nullptr) ? kOk : static_cast<Code>(state_[4]);
          }

          Status(Code code, const std::string& msg, const std::string& msg2) {
            assert(code != kOk);
            const uint32_t len1 = static_cast<uint32_t>(msg.size());
            const uint32_t len2 = static_cast<uint32_t>(msg2.size());
            const uint32_t size = len1 + (len2 ? (2 + len2) : 0);
            char* result = new char[size + 5];
            std::memcpy(result, &size, sizeof(size));
            result[4] = static_cast<char>(code);
            std::memcpy(result + 5, msg.data(), len1);
            if (len2) {
              result[5 + len1] = ':';
              result[6 + len1] = ' ';
              std::memcpy(result + 7 + len1, msg2.data(), len2);
            }
            state_ = result;
          }

        const char* state_;
    };
}


#endif 