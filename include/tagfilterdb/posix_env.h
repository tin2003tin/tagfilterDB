#ifndef TAGFILTERDB_POSIX_ENV_H
#define TAGFILTERDB_POSIX_ENV_H

#include <dirent.h>
#include <fcntl.h>
#include <sys/mman.h>
#ifndef __Fuchsia__
#include <sys/resource.h>
#endif
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <queue>
#include <set>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>

#include "env.h"
#include "status.h"
#include "dataView.h"

namespace tagfilterdb {

    constexpr const size_t kWritableFileBufferSize = 65536;

    // Common flags defined for all posix open operations
    #if defined(HAVE_O_CLOEXEC)
    constexpr const int kOpenBaseFlags = O_CLOEXEC;
    #else
    constexpr const int kOpenBaseFlags = 0;
    #endif  // defined(HAVE_O_CLOEXEC)

    Status PosixError(const std::string& context, int error_number) {
        if (error_number == ENOENT) {
          return Status::NotFound(context, std::strerror(error_number));
        } else {
          return Status::IOError(context, std::strerror(error_number));
        }
    }

    static DataView Basename(const std::string& filename) {
        std::string::size_type separator_pos = filename.rfind('/');
        if (separator_pos == std::string::npos) {
          return DataView(filename);
        }
        // The filename component should not contain a path separator. If it does,
        // the splitting was done incorrectly.
        assert(filename.find('/', separator_pos + 1) == std::string::npos);
    
        return DataView(filename.data() + separator_pos + 1,
                     filename.length() - separator_pos - 1);
    }

    static std::string Dirname(const std::string& filename) {
        std::string::size_type separator_pos = filename.rfind('/');
        if (separator_pos == std::string::npos) {
          return std::string(".");
        }
        // The filename component should not contain a path separator. If it does,
        // the splitting was done incorrectly.
        assert(filename.find('/', separator_pos + 1) == std::string::npos);
    
        return filename.substr(0, separator_pos);
    }

    static bool IsManifest(const std::string& filename) {
        return Basename(filename).starts_with(DataView("MANIFEST"));
    }

    class PosixEnv : public Env {
        public: 
        PosixEnv() {}
        ~PosixEnv() override {
          static const char msg[] =
              "PosixEnv singleton destroyed. Unsupported behavior!\n";
          std::fwrite(msg, 1, sizeof(msg), stderr);
          std::abort();
        }
        Status NewSequentialFile(const std::string& filename,
            SequentialFile** result) override {
            int fd = ::open(filename.c_str(), O_RDONLY | kOpenBaseFlags);
            if (fd < 0) {
                *result = nullptr;
                return PosixError(filename, errno);
            }
        
            *result = new PosixSequentialFile(filename, fd);
            return Status::OK();
        }

        Status NewRandomAccessFile(const std::string& fname,
        RandomAccessFile** result) override {
            return Status::NotSupported("Not implemented");
        }

        Status NewWritableFile(const std::string& filename,
            WritableFile** result) override {
                int fd = ::open(filename.c_str(),
                                O_TRUNC | O_WRONLY | O_CREAT | kOpenBaseFlags, 0644);
                if (fd < 0) {
                *result = nullptr;
                return PosixError(filename, errno);
                }

                *result = new PosixWritableFile(filename, fd);
                return Status::OK();
            }

        Status NewAppendableFile(const std::string& fname,
            WritableFile** result) override {
                return Status::NotSupported("Not implemented");
            }

        bool FileExists(const std::string& fname)override {
            return false;
        }

        Status GetChildren(const std::string& dir, std::vector<std::string>* result)override {
            return Status::NotSupported("Not implemented");
        }
    
        Status RemoveFile(const std::string& fname) override {
            return Status::NotSupported("Not implemented");
        }

        Status DeleteFile(const std::string& fname) override {
            return Status::NotSupported("Not implemented");
        }

        Status CreateDir(const std::string& dirname)override {
            return Status::NotSupported("Not implemented");
        }

        Status RemoveDir(const std::string& dirname) override {
            return Status::NotSupported("Not implemented");
        }

        Status DeleteDir(const std::string& dirname) override {
            return Status::NotSupported("Not implemented");
        }

        Status GetFileSize(const std::string& fname, uint64_t* file_size) override {
            return Status::NotSupported("Not implemented");
        }

        Status RenameFile(const std::string& src,const std::string& target) override {
            return Status::NotSupported("Not implemented");
        }

        Status LockFile(const std::string& fname, FileLock** lock) override {
            return Status::NotSupported("Not implemented");
        }

        Status UnlockFile(FileLock* lock) override {
            return Status::NotSupported("Not implemented");
        }

        Status NewLogger(const std::string& fname, Logger** result) override {
            return Status::NotSupported("Not implemented");
        }

        Status GetTestDirectory(std::string* path) override {
            return Status::NotSupported("Not implemented");
        }
    };
    
    class PosixSequentialFile final : public SequentialFile {
        public:
            PosixSequentialFile(std::string filename, int fd) :
            fd_(fd), filename_(std::move(filename)) {}

            ~PosixSequentialFile() override {close(fd_);}

            Status Read(size_t n, DataView* result, char* scratch) override {
                Status status;
                while (true) {
                    ::ssize_t read_size = ::read(fd_, scratch, n);
                    if (read_size < 0) {  // Read error.
                        if (errno == EINTR) {
                          continue;  // Retry
                        }
                        status = PosixError(filename_, errno);
                        break;
                    }
                    *result = DataView(scratch, read_size);
                    break;
                }
                return status;
            }

            Status Skip(uint64_t n) override {
                if (::lseek(fd_, n, SEEK_CUR) == static_cast<off_t>(-1)) {
                return PosixError(filename_, errno);
                }
                return Status::OK();
            }
        private:
            const int fd_;
            const std::string filename_;
    };

    class PosixWritableFile final : public WritableFile {
        public:
        PosixWritableFile(std::string filename, int fd):
            pos_(0),
            fd_(fd),
            is_manifest_(IsManifest(filename)),
            filename_(std::move(filename)),
            dirname_(Dirname(filename_)) {}
        
        ~PosixWritableFile() override {
                if (fd_ >= 0) {
                  Close();
                }
        }

        Status Append(const DataView& data) override {
            size_t write_size = data.size();
            const char* write_data = data.data();

            size_t copy_size = std::min(write_size, kWritableFileBufferSize);
            std::memcpy(buf_ + pos_, write_data, copy_size);
            write_data += copy_size;
            write_size -= copy_size;
            pos_ += copy_size;
            if (write_size == 0) {
                return Status::OK();
            }

            // Can't fit in buffer, so need to do at least one write.
            Status status = FlushBuffer();
            if (!status.ok()) {
              return status;
            }

            if (write_size < kWritableFileBufferSize) {
                std::memcpy(buf_, write_data, write_size);
                pos_ = write_size;
                return Status::OK();
            }

            return WriteUnbuffered(write_data, write_size);
        }

        Status Close() override {
            Status status = FlushBuffer();
            const int close_result = ::close(fd_);
            if (close_result < 0 && status.ok()) {
              status = PosixError(filename_, errno);
            }
            fd_ = -1;
            return status;
        }
        Status Flush() override { return FlushBuffer(); }

        Status Sync() override {
        // Ensure new files referred to by the manifest are in the filesystem.
        //
        // This needs to happen before the manifest file is flushed to disk, to
        // avoid crashing in a state where the manifest refers to files that are not
        // yet on disk.
        Status status = SyncDirIfManifest();
        if (!status.ok()) {
            return status;
        }
    
        status = FlushBuffer();
        if (!status.ok()) {
            return status;
        }
    
        return SyncFd(fd_, filename_);
        }

        private:
            Status FlushBuffer() {
                Status status = WriteUnbuffered(buf_, pos_);
                pos_ = 0;
                return status;
            }

            Status WriteUnbuffered(const char* data, size_t size) {
                while (size > 0) {
                    ssize_t write_result = ::write(fd_, data, size);
                    if (write_result < 0) {
                        if (errno == EINTR) {
                        continue;  // Retry
                        }
                        return PosixError(filename_, errno);
                    }
                    data += write_result;
                    size -= write_result;
                }
                return Status::OK();
            }

            static Status SyncFd(int fd, const std::string& fd_path) {
            #if HAVE_FULLFSYNC
                // On macOS and iOS, fsync() doesn't guarantee durability past power
                // failures. fcntl(F_FULLFSYNC) is required for that purpose. Some
                // filesystems don't support fcntl(F_FULLFSYNC), and require a fallback to
                // fsync().
                if (::fcntl(fd, F_FULLFSYNC) == 0) {
                    return Status::OK();
                }
            #endif  // HAVE_FULLFSYNC
            
            #if HAVE_FDATASYNC
                bool sync_success = ::fdatasync(fd) == 0;
            #else
                bool sync_success = ::fsync(fd) == 0;
            #endif  // HAVE_FDATASYNC
            
                if (sync_success) {
                    return Status::OK();
                }
                return PosixError(fd_path, errno);
                }
            
            Status SyncDirIfManifest() {
                Status status;
                if (!is_manifest_) {
                    return status;
                }
            
                int fd = ::open(dirname_.c_str(), O_RDONLY | kOpenBaseFlags);
                if (fd < 0) {
                    status = PosixError(dirname_, errno);
                } else {
                    status = SyncFd(fd, dirname_);
                    ::close(fd);
                }
                return status;
            }


          // buf_[0, pos_ - 1] contains data to be written to fd_.
            char buf_[kWritableFileBufferSize];
            size_t pos_;
            int fd_;

            const bool is_manifest_;  // True if the file's name starts with MANIFEST.
            const std::string filename_;
            const std::string dirname_; 
    };

    template <typename EnvType>
    class SingletonEnv {
    public:
    SingletonEnv() {
    #if !defined(NDEBUG)
        env_initialized_.store(true, std::memory_order_relaxed);
    #endif  // !defined(NDEBUG)
        static_assert(sizeof(env_storage_) >= sizeof(EnvType),
                    "env_storage_ will not fit the Env");
        static_assert(std::is_standard_layout_v<SingletonEnv<EnvType>>);
        static_assert(
            offsetof(SingletonEnv<EnvType>, env_storage_) % alignof(EnvType) == 0,
            "env_storage_ does not meet the Env's alignment needs");
        static_assert(alignof(SingletonEnv<EnvType>) % alignof(EnvType) == 0,
                    "env_storage_ does not meet the Env's alignment needs");
        new (env_storage_) EnvType();
    }
    ~SingletonEnv() = default;

    SingletonEnv(const SingletonEnv&) = delete;
    SingletonEnv& operator=(const SingletonEnv&) = delete;

    Env* env() { return reinterpret_cast<Env*>(&env_storage_); }

    static void AssertEnvNotInitialized() {
    #if !defined(NDEBUG)
        assert(!env_initialized_.load(std::memory_order_relaxed));
    #endif  // !defined(NDEBUG)
    }

    private:
    alignas(EnvType) char env_storage_[sizeof(EnvType)];
    #if !defined(NDEBUG)
    static std::atomic<bool> env_initialized_;
    #endif  // !defined(NDEBUG)
    };

    #if !defined(NDEBUG)
    template <typename EnvType>
    std::atomic<bool> SingletonEnv<EnvType>::env_initialized_;
    #endif  // !defined(NDEBUG)

    using PosixDefaultEnv = SingletonEnv<PosixEnv>;

      
    Env* Env::Default() {
        static PosixDefaultEnv env_container;
        return env_container.env();
    }
}

#endif 