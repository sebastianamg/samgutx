#pragma once
// #include <chrono>
#include <fmt/chrono.h>
#include <iomanip>
#include <iostream>
#include <stdio.h>

#define TIMESTAMP (fmt::to_string(fmt::format("{:%S}", std::chrono::system_clock::now().time_since_epoch())).c_str())

#ifdef DEBUGGER_LOG
    #define LOG(...) { \
        fprintf(stderr, "[%s | %d @ %s] ", __FILE__, __LINE__, TIMESTAMP); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"); \
    }
#else
    #define LOG(...) {}
#endif