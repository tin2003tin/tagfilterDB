#ifndef TAGFILTERDB_ENV_H
#define TAGFILTERDB_ENV_H

#include <cstdarg>
#include <cstdint>
#include <string>
#include <vector>

namespace tagfilterdb {
    class Status;
    class FileLock;
    class Logger;
    class RandomAccessFile;
    class SequentialFile;
    class DataView;
    class WritableFile;

    class Env {
        public:

        Env() = default;

        Env(const Env&) = delete;
        Env& operator=(const Env&) = delete;

        virtual ~Env() = default;

        virtual Status NewSequentialFile(const std::string& fname,
            SequentialFile** result) = 0;

        virtual Status NewRandomAccessFile(const std::string& fname,
            RandomAccessFile** result) = 0;

        virtual Status NewWritableFile(const std::string& fname,
            WritableFile** result) = 0;

        virtual Status NewAppendableFile(const std::string& fname,
            WritableFile** result) = 0;

        virtual bool FileExists(const std::string& fname) = 0;

        virtual Status GetChildren(const std::string& dir, std::vector<std::string>* result) = 0;
    
        virtual Status RemoveFile(const std::string& fname) = 0;

        virtual Status DeleteFile(const std::string& fname) = 0;

        virtual Status CreateDir(const std::string& dirname) = 0;

        virtual Status RemoveDir(const std::string& dirname) = 0;

        virtual Status DeleteDir(const std::string& dirname) = 0;

        virtual Status GetFileSize(const std::string& fname, uint64_t* file_size) = 0;

        virtual Status RenameFile(const std::string& src,const std::string& target) = 0;

        virtual Status LockFile(const std::string& fname, FileLock** lock) = 0;

        virtual Status UnlockFile(FileLock* lock) = 0;

        virtual Status NewLogger(const std::string& fname, Logger** result) = 0;

        virtual Status GetTestDirectory(std::string* path) = 0;
    };

    class SequentialFile {
        public:
        SequentialFile() = default;

        SequentialFile(const SequentialFile&) = delete;
        SequentialFile& operator=(const SequentialFile&) = delete;

        virtual ~SequentialFile() = default;

        virtual Status Read(size_t n, DataView* result, char* scratch) = 0;

        virtual Status Skip(uint64_t n) = 0;
    };

    class RandomAccessFile {
        public:
        RandomAccessFile() = default;

        
        RandomAccessFile(const RandomAccessFile&) = delete;
        RandomAccessFile& operator=(const RandomAccessFile&) = delete;

        virtual ~RandomAccessFile() = default;

        virtual Status Read(uint64_t offset, size_t n, DataView* result,
            char* scratch) const = 0;
    };

    class WritableFile {
        public:
         WritableFile() = default;
       
         WritableFile(const WritableFile&) = delete;
         WritableFile& operator=(const WritableFile&) = delete;
       
         virtual ~WritableFile() = default;
       
         virtual Status Append(const DataView& data) = 0;
         virtual Status Close() = 0;
         virtual Status Flush() = 0;
         virtual Status Sync() = 0;
       };

       class  Logger {
        public:
         Logger() = default;
       
         Logger(const Logger&) = delete;
         Logger& operator=(const Logger&) = delete;
       
         virtual ~Logger() = default;
       
         virtual void Logv(const char* format, std::va_list ap) = 0;
       };

       class FileLock {
        public:
         FileLock() = default;
       
         FileLock(const FileLock&) = delete;
         FileLock& operator=(const FileLock&) = delete;
       
         virtual ~FileLock() = default;
       };
}

#endif