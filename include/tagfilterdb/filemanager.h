#ifndef TAGFILTERDB_FILEMANAGER_H
#define TAGFILTERDB_FILEMANAGER_H

#include "record.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <cassert>

namespace tagfilterdb {
    constexpr size_t FILE_SIZE = 1024 * 1024; // 1 MB for each file
    constexpr size_t METADATA_SIZE = sizeof(size_t); // Metadata stores the number of records


    class FileManager {
        std::string baseFilename_;
        size_t recordsPerFile_ = FILE_SIZE / RECORD_SIZE;

        public:
        // Get the name of a specific file
        std::string GetFilename(size_t fileIndex) const {
            std::ostringstream oss;
            oss << baseFilename_ << "_" << fileIndex;
            return oss.str();
        }

        std::string GetMetaFilename() const {
            std::ostringstream oss;
            oss << baseFilename_ << "_meta";
            return oss.str();
        }

        // Find the total number of records across files
        size_t TotalRecords() {
            size_t totalRecords = 0;
            for (size_t fileIndex = 0;; ++fileIndex) {
                std::ifstream in(GetFilename(fileIndex), std::ios::binary);
                if (!in) break;
                in.seekg(0, std::ios::end);
                totalRecords += in.tellg() / RECORD_SIZE;
            }
            return totalRecords;
        }


        // Constructor
        explicit FileManager(const std::string& baseFilename)
            : baseFilename_(baseFilename) {}

        void Save(const DataRecord& record) {
            size_t totalRecords = readMetadata();
            size_t fileIndex = totalRecords / recordsPerFile_;
            size_t recordOffset = (totalRecords % recordsPerFile_) * RECORD_SIZE;

            std::ofstream out(GetFilename(fileIndex), std::ios::binary | std::ios::app);
            if (!out) {
                throw std::runtime_error("Failed to open file for saving.");
            }

            record.Serialize(out);

            writeMetadata(totalRecords + 1);
        }

        // Get a specific record by index
        DataRecord GetRecord(size_t recordIndex) {
            size_t totalRecords = readMetadata();
            if (recordIndex >= totalRecords) {
                return DataRecord();
            }

            size_t fileIndex = recordIndex / recordsPerFile_;
            size_t recordOffset = (recordIndex % recordsPerFile_) * RECORD_SIZE;
            std::ifstream in(GetFilename(fileIndex), std::ios::binary);
            if (!in) {
                throw std::runtime_error("Failed to open file for reading.");
            }

            in.seekg(recordOffset);
            if (!in) {
                throw std::runtime_error("Failed to seek to record position.");
            }

            return DataRecord::Deserialize(in);
        }

        // Rewrite a specific record by index
        void ReWriteRecord(size_t recordIndex, const DataRecord& newRecord) {
            size_t fileIndex = recordIndex / recordsPerFile_;
            size_t recordOffset = (recordIndex % recordsPerFile_) * RECORD_SIZE;

            std::ofstream file(GetFilename(fileIndex), std::ios::binary | std::ios::in | std::ios::out);
            if (!file) {
                throw std::runtime_error("Failed to open file for updating.");
            }

            file.seekp(recordOffset);
            newRecord.Serialize(file);
        }

        std::vector<DataRecord> Scan() {
            std::vector<DataRecord> records;

            for (size_t fileIndex = 0;; ++fileIndex) {
                std::ifstream in(GetFilename(fileIndex), std::ios::binary);
                if (!in) break;  // No more files to read

                while (in.peek() != EOF) {
                    DataRecord record = DataRecord::Deserialize(in);
                    records.push_back(record); 
                }
            }

            return records;
        }

        void Clean() {
            std::string metaFile = GetMetaFilename();
            std::remove(metaFile.c_str());
            for (size_t fileIndex = 0;; ++fileIndex) {
                std::string filename = GetFilename(fileIndex);
                if (std::remove(filename.c_str()) != 0) {
                    break; // Stop when a file does not exist
                }
            }   
        }

        private: 
            void writeMetadata(size_t totalRecords) {
                // Create the metadata file name
                std::ostringstream metafileOss;
                metafileOss << baseFilename_ << "_meta";
                std::ofstream out(metafileOss.str(), std::ios::binary | std::ios::trunc);
                if (!out) {
                    throw std::runtime_error("Failed to open metadata file for writing.");
                }
                out.write(reinterpret_cast<const char*>(&totalRecords), METADATA_SIZE);
            }

            size_t readMetadata() {
                // Create the metadata file name
                std::ostringstream oss;
                oss << baseFilename_ << "_meta";
                std::ifstream in(oss.str(), std::ios::binary);
                
                // Check if file exists and can be opened
                if (!in) {
                    // File doesn't exist, so create and initialize it
                    std::ofstream out(oss.str(), std::ios::binary | std::ios::trunc);
                    if (!out) {
                        throw std::runtime_error("Failed to create metadata file.");
                    }
                    size_t defaultTotalRecords = 0;  // Default value for total records
                    out.write(reinterpret_cast<const char*>(&defaultTotalRecords), METADATA_SIZE);
                    return defaultTotalRecords;  // Return default value when creating new file
                }

                // If file exists, read the totalRecords
                size_t totalRecords = 0;
                in.read(reinterpret_cast<char*>(&totalRecords), METADATA_SIZE);
                return totalRecords;
            }
    };

}

#endif