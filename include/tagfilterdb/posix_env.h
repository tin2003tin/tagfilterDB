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

    int g_open_read_only_file_limit = -1;

    constexpr const int kDefaultMmapLimit = (sizeof(void*) >= 8) ? 1000 : 0;

    int g_mmap_limit = kDefaultMmapLimit;
    
    int MaxMmaps() { return g_mmap_limit; }

    constexpr const size_t kWritableFileBufferSize = 65536;

    // Common flags defined for all posix open operations
    #if defined(HAVE_O_CLOEXEC)
    constexpr const int kOpenBaseFlags = O_CLOEXEC;
    #else
    constexpr const int kOpenBaseFlags = 0;
    #endif  // defined(HAVE_O_CLOEXEC)

    int MaxOpenFiles() {
    if (g_open_read_only_file_limit >= 0) {
        return g_open_read_only_file_limit;
    }
    #ifdef __Fuchsia__
    // Fuchsia doesn't implement getrlimit.
    g_open_read_only_file_limit = 50;
    #else
    struct ::rlimit rlim;
    if (::getrlimit(RLIMIT_NOFILE, &rlim)) {
        // getrlimit failed, fallback to hard-coded default.
        g_open_read_only_file_limit = 50;
    } else if (rlim.rlim_cur == RLIM_INFINITY) {
        g_open_read_only_file_limit = std::numeric_limits<int>::max();
    } else {
        // Allow use of 20% of available file descriptors for read-only files.
        g_open_read_only_file_limit = rlim.rlim_cur / 5;
    }
    #endif
    return g_open_read_only_file_limit;
    }

    // Helper class to limit resource usage to avoid exhaustion.
    // Currently used to limit read-only file descriptors and mmap file usage
    // so that we do not run out of file descriptors or virtual memory, or run into
    // kernel performance problems for very large databases.
    class Limiter {
    public:
        // Limit maximum number of resources to |max_acquires|.
        Limiter(int max_acquires)
            :
    #if !defined(NDEBUG)
            max_acquires_(max_acquires),
    #endif  // !defined(NDEBUG)
            acquires_allowed_(max_acquires) {
        assert(max_acquires >= 0);
        }

        Limiter(const Limiter&) = delete;
        Limiter operator=(const Limiter&) = delete;

        // If another resource is available, acquire it and return true.
        // Else return false.
        bool Acquire() {
        int old_acquires_allowed =
            acquires_allowed_.fetch_sub(1, std::memory_order_relaxed);

        if (old_acquires_allowed > 0) return true;

        int pre_increment_acquires_allowed =
            acquires_allowed_.fetch_add(1, std::memory_order_relaxed);

        // Silence compiler warnings about unused arguments when NDEBUG is defined.
        (void)pre_increment_acquires_allowed;
        // If the check below fails, Release() was called more times than acquire.
        assert(pre_increment_acquires_allowed < max_acquires_);

        return false;
        }

        // Release a resource acquired by a previous call to Acquire() that returned
        // true.
        void Release() {
        int old_acquires_allowed =
            acquires_allowed_.fetch_add(1, std::memory_order_relaxed);

        // Silence compiler warnings about unused arguments when NDEBUG is defined.
        (void)old_acquires_allowed;
        // If the check below fails, Release() was called more times than acquire.
        assert(old_acquires_allowed < max_acquires_);
        }

    private:
    #if !defined(NDEBUG)
        // Catches an excessive number of Release() calls.
        const int max_acquires_;
    #endif  // !defined(NDEBUG)

        // The number of available resources.
        //
        // This is a counter and is not tied to the invariants of any other class, so
        // it can be operated on safely using std::memory_order_relaxed.
        std::atomic<int> acquires_allowed_;
    };

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

    class PosixRandomAccessFile : public RandomAccessFile {
        public:
        PosixRandomAccessFile(std::string filename, int fd, Limiter* fd_limiter)
        : has_permanent_fd_(fd_limiter->Acquire()),
        fd_(has_permanent_fd_ ? fd : -1),
        fd_limiter_(fd_limiter),
        filename_(std::move(filename)) {
            if (!has_permanent_fd_) {
                assert(fd == -1);
                ::close(fd);
            }
        }

        ~PosixRandomAccessFile() override {
            if (has_permanent_fd_) {
                assert(fd_ != -1);
                ::close(fd_);
                fd_limiter_->Release();
            }
        }

        Status Read(uint64_t offset, size_t n, DataView* result,
            char* scratch) const override {
            int fd = fd_;
            if(!has_permanent_fd_) {
                fd = ::open(filename_.c_str(), O_RDONLY | kOpenBaseFlags);
                if (fd < 0) {
                    return PosixError(filename_, errno);
                }
            }
            assert(fd != -1);
            Status status;
            ssize_t read_size = ::pread(fd, scratch, n, static_cast<off_t>(offset));
            *result = DataView(scratch, (read_size < 0) ? 0 : read_size);
            if (read_size < 0) {
                // An error: return a non-ok status.
                status = PosixError(filename_, errno);
              }
              if (!has_permanent_fd_) {
                // Close the temporary file descriptor opened earlier.
                assert(fd != fd_);
                ::close(fd);
              }
              return status;
        }

        private:
        const bool has_permanent_fd_;  // If false, the file is opened on every read.
        const int fd_;                 // -1 if has_permanent_fd_ is false.
        Limiter* const fd_limiter_;
        const std::string filename_;
    };

    class PosixMmapReadableFile final : public RandomAccessFile {
    public:
        // mmap_base[0, length-1] points to the memory-mapped contents of the file. It
        // must be the result of a successful call to mmap(). This instances takes
        // over the ownership of the region.
        //
        // |mmap_limiter| must outlive this instance. The caller must have already
        // acquired the right to use one mmap region, which will be released when this
        // instance is destroyed.
        PosixMmapReadableFile(std::string filename, char* mmap_base, size_t length,
                            Limiter* mmap_limiter)
            : mmap_base_(mmap_base),
            length_(length),
            mmap_limiter_(mmap_limiter),
            filename_(std::move(filename)) {}
    
        ~PosixMmapReadableFile() override {
        ::munmap(static_cast<void*>(mmap_base_), length_);
        mmap_limiter_->Release();
        }
    
        Status Read(uint64_t offset, size_t n, DataView* result,
                    char* scratch) const override {
        if (offset + n > length_) {
            *result = DataView();
            return PosixError(filename_, EINVAL);
        }
    
        *result = DataView(mmap_base_ + offset, n);
        return Status::OK();
        }
    
    private:
        char* const mmap_base_;
        const size_t length_;
        Limiter* const mmap_limiter_;
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

    class PosixEnv : public Env {
        public: 
        PosixEnv() : mmap_limiter_(MaxMmaps()),
        fd_limiter_(MaxOpenFiles()) {}
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

        Status NewRandomAccessFile(const std::string& filename,
        RandomAccessFile** result) override {
            *result = nullptr;
            int fd = ::open(filename.c_str(), O_RDONLY | kOpenBaseFlags);
            if (fd < 0) {
              return PosixError(filename, errno);
            }
        
            if (!mmap_limiter_.Acquire()) {
              *result = new PosixRandomAccessFile(filename, fd, &fd_limiter_);
              return Status::OK();
            }
        
            uint64_t file_size;
            Status status = GetFileSize(filename, &file_size);
            if (status.ok()) {
              void* mmap_base =
                  ::mmap(/*addr=*/nullptr, file_size, PROT_READ, MAP_SHARED, fd, 0);
              if (mmap_base != MAP_FAILED) {
                *result = new PosixMmapReadableFile(filename,
                                                    reinterpret_cast<char*>(mmap_base),
                                                    file_size, &mmap_limiter_);
              } else {
                status = PosixError(filename, errno);
              }
            }
            ::close(fd);
            if (!status.ok()) {
              mmap_limiter_.Release();
            }
            return status;
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

        Status NewAppendableFile(const std::string& filename,
            WritableFile** result) override {
                return Status::NotSupported("Not implemented");
            }

        bool FileExists(const std::string& filename)override {
            return ::access(filename.c_str(), F_OK);
        }

        Status GetChildren(const std::string& directory_path, std::vector<std::string>* result)override {
            result->clear();
            ::DIR* dir = ::opendir(directory_path.c_str());
            if (dir == nullptr) {
              return PosixError(directory_path, errno);
            }
            struct ::dirent* entry;
            while ((entry = ::readdir(dir)) != nullptr) {
              result->emplace_back(entry->d_name);
            }
            ::closedir(dir);
            return Status::OK();
        }
    
        Status RemoveFile(const std::string& filename) override {
            if (::unlink(filename.c_str()) != 0) {
                return PosixError(filename, errno);
              }
              return Status::OK();        
        }

        Status CreateDir(const std::string& dirname)override {
            if (::mkdir(dirname.c_str(), 0755) != 0) {
                return PosixError(dirname, errno);
              }
              return Status::OK();        }

        Status RemoveDir(const std::string& dirname) override {
            if (::rmdir(dirname.c_str()) != 0) {
                return PosixError(dirname, errno);
              }
              return Status::OK();        }

        Status GetFileSize(const std::string& filename, uint64_t* size) override {
            struct ::stat file_stat;
            if (::stat(filename.c_str(), &file_stat) != 0) {
              *size = 0;
              return PosixError(filename, errno);
            }
            *size = file_stat.st_size;
            return Status::OK();        }

        Status RenameFile(const std::string& from,const std::string& to) override {
            if (std::rename(from.c_str(), to.c_str()) != 0) {
                return PosixError(from, errno);
              }
              return Status::OK();        
        }

        Status LockFile(const std::string& filename, FileLock** lock) override {
            return Status::NotSupported("Not implemented");
        }

        Status UnlockFile(FileLock* lock) override {
            return Status::NotSupported("Not implemented");
        }

        Status NewLogger(const std::string& filename, Logger** result) override {
            return Status::NotSupported("Not implemented");
        }

        Status GetTestDirectory(std::string* path) override {
            return Status::NotSupported("Not implemented");
        }

        private:
        Limiter mmap_limiter_;  // Thread-safe.
        Limiter fd_limiter_;    // Thread-safe.
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