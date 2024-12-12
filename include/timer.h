#pragma once

#include <iostream>
#include <chrono>
#include <string>

class ScopedTimer {
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
    std::string function_name;

public:
    ScopedTimer(const std::string& name) : function_name(name) {
        start_time = std::chrono::high_resolution_clock::now();
    }

    ~ScopedTimer() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
        std::cout << "Elapsed time for " << function_name << ": " << elapsed_time << " µs" << std::endl;
    }
};