#pragma once
#include <cstdio>
#include <stdexcept>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

/**
 * ---------------------------------------------------------------
 * Released under the 2-Clause BSD License 
 * (a.k.a. Simplified BSD License or FreeBSD License)
 * @note [link https://opensource.org/license/bsd-2-clause/ BSD-2-Clause]
 * ---------------------------------------------------------------
 * 
 * @copyright (c) 2023 Sebastián AMG (@sebastianamg)
 * 
 * Redistribution and use in source and binary forms, with or 
 * without modification, are permitted provided 
 * that the following conditions are met:
 *  1.  Redistributions of source code must retain the above 
 *      copyright notice, this list of conditions and the 
 *      following disclaimer.
 * 
 *  2.  Redistributions in binary form must reproduce the 
 *      above copyright notice, this list of conditions and
 *      the following disclaimer in the documentation and/or 
 *      other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
 * CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
 * THE POSSIBILITY OF SUCH DAMAGE. 
 */

 /** 
 * @brief Wrap for c-logger
 * [link https://github.com/adaxiik/c-logger c-logger]
 * @note Since this is one-header file, `c-logger` has been replicated within this file. 
 */

#ifdef __cplusplus
    #include <string>
#endif

#define LOGGER_LEVEL_DEBUG 0
#define LOGGER_LEVEL_INFO 1
#define LOGGER_LEVEL_WARNING 2
#define LOGGER_LEVEL_ERROR 3
#define LOGGER_LEVEL_FATAL 4

#define LOGGER_COLORS_OFF 0
#define LOGGER_COLORS_ON 1

#define LOGGER_COLOR_RESET "\033[0m"
#define LOGGER_COLOR_RED "\033[31m"
#define LOGGER_COLOR_GREEN "\033[32m"
#define LOGGER_COLOR_YELLOW "\033[33m"
#define LOGGER_COLOR_BLUE "\033[34m"

static const char* g_levelStrings[] = {
    "[DEBUG]",
    "[INFO]",
    "[WARNING]",
    "[ERROR]",
    "[FATAL]"
};

static const char* g_levelColors[] = {
    LOGGER_COLOR_BLUE,
    LOGGER_COLOR_GREEN,
    LOGGER_COLOR_YELLOW,
    LOGGER_COLOR_RED,
    LOGGER_COLOR_RED
};

#define LOGGER_OUTPUT_STDOUT stdout
#define LOGGER_OUTPUT_STDERR stderr

#ifndef LOGGER_SETTINGS_LEVEL
    #define LOGGER_SETTINGS_LEVEL LOGGER_LEVEL_DEBUG
#endif

#ifndef LOGGER_SETTINGS_COLORS
    #define LOGGER_SETTINGS_COLORS LOGGER_COLORS_ON
#endif


#define LOGGER_LOG(file,level,fmt,...) do { \
    if (level >= LOGGER_SETTINGS_LEVEL) { \
        fprintf(file, "%s%s%s: ", LOGGER_SETTINGS_COLORS ? g_levelColors[level] : "", g_levelStrings[level], LOGGER_SETTINGS_COLORS ? LOGGER_COLOR_RESET : ""); \
        fprintf(file, fmt, ##__VA_ARGS__); \
        fprintf(file, "\n"); \
    } \
    } while (0)


#define LOG_DEBUG(fmt,...) LOGGER_LOG(LOGGER_OUTPUT_STDOUT, LOGGER_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt,...) LOGGER_LOG(LOGGER_OUTPUT_STDOUT, LOGGER_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt,...) LOGGER_LOG(LOGGER_OUTPUT_STDOUT, LOGGER_LEVEL_WARNING, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt,...) LOGGER_LOG(LOGGER_OUTPUT_STDERR, LOGGER_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define LOG_FATAL(fmt,...) LOGGER_LOG(LOGGER_OUTPUT_STDERR, LOGGER_LEVEL_FATAL, fmt, ##__VA_ARGS__); exit(1)

#define LOG_WATCH_INT(variable) LOG_DEBUG(#variable " = %d (%s:%d)", variable, __FILE__, __LINE__)
#define LOG_WATCH_UINT(variable) LOG_DEBUG(#variable " = %u (%s:%d)", variable, __FILE__, __LINE__)
#define LOG_WATCH_LONG(variable) LOG_DEBUG(#variable " = %ld (%s:%d)", variable, __FILE__, __LINE__)
#define LOG_WATCH_ULONG(variable) LOG_DEBUG(#variable " = %lu (%s:%d)", variable, __FILE__, __LINE__)
#define LOG_WATCH_FLOAT(variable) LOG_DEBUG(#variable " = %f (%s:%d)", variable, __FILE__, __LINE__)
#define LOG_WATCH_DOUBLE(variable) LOG_DEBUG(#variable " = %lf (%s:%d)", variable, __FILE__, __LINE__)
#define LOG_WATCH_STRING(variable) LOG_DEBUG(#variable " = %s (%s:%d)", variable, __FILE__, __LINE__)
#define LOG_WATCH_BOOL(variable) LOG_DEBUG(#variable " = %s (%s:%d)", variable ? "true" : "false", __FILE__, __LINE__)
#define LOG_WATCH_HEX(variable) LOG_DEBUG(#variable " = 0x%x (%s:%d)", variable, __FILE__, __LINE__)
#define LOG_WATCH_PTR(variable) LOG_DEBUG(#variable " = %p (%s:%d)", variable, __FILE__, __LINE__)

#ifdef __cplusplus
    #define LOG_WATCH(variable) do { \
        LOG_DEBUG(#variable " = %s (%s:%d)", std::to_string(variable).c_str(), __FILE__, __LINE__); \
        } while (0)
#endif

namespace samg {
    class Logger {
        public:
            enum LoggerStatus {
                OFF,
                ON
            } status;

            enum LoggerFileMode {
                NEW,
                APPEND
            };

            Logger() {
                this->outputfile = nullptr;
                this->dual_output = false;
                this->status = LoggerStatus::ON;
            }

            Logger(const std::string file_name, const bool dual_output = false, const LoggerStatus status = LoggerStatus::ON, const LoggerFileMode mode = LoggerFileMode::NEW):
            status(status), dual_output(dual_output) {
                if(mode == LoggerFileMode::NEW){
                    this->outputfile = std::fopen(file_name.data(), "w");
                } else if(mode == LoggerFileMode::APPEND){
                    this->outputfile = std::fopen(file_name.data(), "a");
                } else {
                    this->outputfile = nullptr;
                    this->status = LoggerStatus::OFF;
                }
            }

            void print(const std::string msg) {
                if( this->status == LoggerStatus::ON) {
                    if( this->outputfile == nullptr || dual_output ) {
                        std::cout << msg << std::endl;
                    }
                    if( this->outputfile != nullptr ) {
                        fprintf(this->outputfile,"%s",msg.data());
                    }
                }
            }

            void debug(const std::string msg) {
                this->_print_(msg, LOGGER_LEVEL_DEBUG, this->outputfile, this->status, this->dual_output);
            }

            void info(const std::string msg) {
                this->_print_(msg, LOGGER_LEVEL_INFO, this->outputfile, this->status, this->dual_output);
            }

            void warn(const std::string msg) {
                this->_print_(msg, LOGGER_LEVEL_WARNING, this->outputfile, this->status, this->dual_output);
            }

            void error(const std::string msg) {
                this->_print_(msg, LOGGER_LEVEL_ERROR, this->outputfile, this->status, this->dual_output);
            }

            void fatal(const std::string msg) {
                this->_print_(msg, LOGGER_LEVEL_FATAL, this->outputfile, this->status, this->dual_output);
            }

            void close() {
                fclose(this->outputfile);
            }

        private:
            FILE* outputfile;
            bool dual_output;

            void _print_(const std::string msg, const std::uint8_t level, FILE* file, const LoggerStatus status, const bool dual_output) {
                if( status == LoggerStatus::ON) {
                    if( level < 5 ){
                        if( file == nullptr || dual_output ) {
                            LOGGER_LOG(stdout, level, "%s", msg.data());
                        }
                        if( file != nullptr ) {
                            LOGGER_LOG(file, level, "%s", msg.data());
                        }
                    }
                }
            }
    };
}