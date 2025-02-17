#pragma once

#include "tagfilterdb/posix_env.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>


int posix_env_example() {
    const std::string filename = "test.txt";

    tagfilterdb::Env* env = tagfilterdb::Env::Default(); 

    tagfilterdb::WritableFile* writeFile;
    env->NewWritableFile(filename, &writeFile);

    std::string data = "Hello, world!\n";
    tagfilterdb::DataView dataView(data.c_str(), data.size());

    tagfilterdb::Status status = writeFile->Append(dataView);
    if (!status.ok()) {
        std::cerr << "Error writing to file\n";
        return 1;
    }

    status = writeFile->Sync();
    if (!status.ok()) {
        std::cerr << status.ToString() << std::endl;    
        return 1;
    }

    status = writeFile->Close();
    if (!status.ok()) {
        std::cerr << status.ToString() << std::endl;  
        return 1;
    }

    std::cout << "File written and synced successfully.\n";

    tagfilterdb::SequentialFile* seqFile;
    env->NewSequentialFile(filename, &seqFile);

    char scratch[1024];
    tagfilterdb::DataView result;

    status = seqFile->Read(512, &result, scratch);
    if (status.ok()) {
        std::cout << "Data read: " << result.ToString() << std::endl;
    } else {
        std::cerr << status.ToString() << std::endl;    
        return 1;
    }

    return 0;
}
