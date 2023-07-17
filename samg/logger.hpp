#pragma once
#include <logger.h>
#include <cstdio>
#include <stdexcept>

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