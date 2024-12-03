#ifndef TAGFILTERDB_RECORD
#define TAGFILTERDB_RECORD

#include <string>
#include <stdexcept>
#include <cstring>
#include <fstream>
#include <iostream>

namespace tagfilterdb {
    constexpr size_t RECORD_SIZE = 1024; 

    class DataRecord {
        char data_[RECORD_SIZE] = {0};

    public:
        explicit DataRecord(const std::string& data) {
            if (data.size() > RECORD_SIZE) {
                throw std::runtime_error("Data exceeds record size limit.");
            }
            std::memcpy(data_, data.c_str(), data.size());
        }

        DataRecord() = default;

        void Serialize(std::ostream& out) const {
            out.write(data_, RECORD_SIZE);
        }

        static DataRecord Deserialize(std::istream& in) {
            DataRecord record;
            in.read(record.data_, RECORD_SIZE);
            return record;
        }

        void Print() const {
            std::cout << std::string(data_, strnlen(data_, RECORD_SIZE)) << std::endl;
        }
    };

}


#endif