#pragma once

#include "filemanager.h"

using namespace tagfilterdb;

void setup(FileManager &fileManager) {
    fileManager.Clean();
    // Save initial records
    for (int i = 0; i < 10000; ++i) {
        std::ostringstream oss;
        oss << "Record " << i;
        DataRecord record(oss.str());
        fileManager.Save(record);
    }
}

void testClean(FileManager& fileManager) {
    fileManager.Clean();
    assert(fileManager.TotalRecords() == 0);
    std::cout << "Test Passed: clean function works correctly.\n";
}

void testScan(FileManager& fileManager) {
    std::vector<DataRecord> records = fileManager.Scan();
    size_t expectedRecords = 10000;

    assert(records.size() == expectedRecords);
    std::cout << "Test Passed: scan function returns the correct number of records.\n";
}

void testSaveAndTotalRecords(FileManager& fileManager) {
    assert(fileManager.TotalRecords() == 10000);
    std::cout << "Test Passed: save and TotalRecords work correctly.\n";
}

void testGetRecord(FileManager& fileManager) {
    DataRecord record = fileManager.GetRecord(1500);
    std::ostringstream expectedContent;
    expectedContent << "Record 1500";

    std::ostringstream actualContent;
    record.Serialize(actualContent);

    assert(actualContent.str().substr(0, expectedContent.str().size()) == expectedContent.str());
    std::cout << "Test Passed: getRecord retrieves the correct record.\n";
}

void testReWriteRecord(FileManager& fileManager) {
    DataRecord newRecord("Updated Record 1500");

    fileManager.ReWriteRecord(1500, newRecord);

    DataRecord record = fileManager.GetRecord(1500);
    std::ostringstream expectedContent;
    expectedContent << "Updated Record 1500";

    std::ostringstream actualContent;
    record.Serialize(actualContent);
    assert(actualContent.str().substr(0, expectedContent.str().size()) == expectedContent.str());
    std::cout << "Test Passed: reWriteRecord updates the record correctly.\n";
}

int filemanager_test() {
    try {
        std::cout << "Trying test\n";
        std::cout << "Creating file...\n";
        FileManager fileManager("data_file");

        // Set up the file manager with initial data
        setup(fileManager);

        // Run all tests
        testSaveAndTotalRecords(fileManager);
        testScan(fileManager);
        testGetRecord(fileManager);
        testReWriteRecord(fileManager);
        testClean(fileManager);

        std::cout << "All tests passed successfully.\n";
    } catch (const std::exception& e) {
        std::cerr << "Test Failed: " << e.what() << '\n';
    }

    return 0;
}