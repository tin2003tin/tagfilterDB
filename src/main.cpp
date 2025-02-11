#include "tagfilterdb/posix_env.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>

int main() {
    const std::string filename = "test.txt";

    int fd = ::open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        std::cerr << "Error opening file for writing\n";
        return 1;
    }

    tagfilterdb::PosixWritableFile file(filename, fd);

    std::string data = "Hello, world!\n";
    tagfilterdb::DataView dataView(data.c_str(), data.size());

    tagfilterdb::Status status = file.Append(dataView);
    if (!status.ok()) {
        std::cerr << "Error writing to file\n";
        return 1;
    }

    status = file.Sync();
    if (!status.ok()) {
        std::cerr << "Error syncing file\n";
        return 1;
    }

    status = file.Close();
    if (!status.ok()) {
        std::cerr << "Error closing file\n";
        return 1;
    }

    std::cout << "File written and synced successfully.\n";

    fd = ::open(filename.c_str(), O_RDONLY);
    if (fd < 0) {
        std::cerr << "Error opening file for reading\n";
        return 1;
    }

    tagfilterdb::PosixSequentialFile seqFile(filename, fd);

    char scratch[1024];
    tagfilterdb::DataView result;

    status = seqFile.Read(512, &result, scratch);
    if (status.ok()) {
        std::cout << "Data read: " << result.ToString() << std::endl;
    } else {
        std::cerr << "Read error: " << status.ToString() << std::endl;
    }

    close(fd);

    return 0;
}
