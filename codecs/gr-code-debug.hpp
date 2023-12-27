#pragma once
// #include <chrono>
#include <fmt/chrono.h>
#include <iomanip>
#include <iostream>
#include <stdio.h>

#define TIMESTAMP_FORMAT ("%Y-%m-%d %H:%M:%S.")
#define TIMESTAMP (std::string(__TIMESTAMP__).substr(20, 19) + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) % 1000))

#ifdef DEBUG_OFFLINERICERUNSWRITER_ADD
#define PRINT_OFFLINERICERUNSWRITER_ADD(...) { \
        fprintf(stderr, "[%s] %s@%d:OfflineRiceRunsWriter/add> ", TIMESTAMP, __FILE__, __LINE__); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"); \
    }
#endif
// fprintf(stderr, TIMESTAMP + " --- " + __FILE__ << ":" << __LINE__ << ": OfflineRiceRunsWriter/add> " << __VA_ARGS__ << std::endl