#pragma once
#include <fcntl.h>
#include <unistd.h>
#include <chrono>
#include <map>
#include <cmath>
#include <string>
#include <string.h>
#include <functional>

namespace samg {
    namespace profiler {

        /**
         * @brief This class allows measure exection time and/or memory usage of a given function.
         * 
         * @tparam TimeT 
         * @tparam ClockT 
         */
        template <class TimeT  = std::chrono::nanoseconds, class ClockT = std::chrono::steady_clock>
        class Profiler {
            public:
                static const std::string EXECUTION_TIME;
                static const std::string MEMORY_USAGE;
                static const std::string INITIAL_MEMORY;
                static const std::string FINAL_MEMORY;

                /**
                 * @brief This function measures execution time of a given function. 
                 * 
                 * @tparam R 
                 * @tparam F 
                 * @tparam Args 
                 * @param func 
                 * @param args 
                 * @return R 
                 */
                template <typename R, class F, class ...Args>
                R measure_time(F&& func, Args&&... args) {
                    auto start = ClockT::now();
                    R ans = std::invoke(std::forward<F>(func), std::forward<Args>(args)...);
                    this->profile[Profiler::EXECUTION_TIME] = std::to_string( std::chrono::duration_cast<TimeT>(ClockT::now()-start).count() );
                    return ans;
                }

                /**
                 * @brief This function measures memory usage of a given function. 
                 * 
                 * @tparam R 
                 * @tparam F 
                 * @tparam Args 
                 * @param func 
                 * @param args 
                 * @return R 
                 */
                template <typename R, class F, class ...Args>
                R measure_memory(F&& func, Args&&... args) {
                    const pid_t this_process = getpid();
                    std::uint64_t beginning_memory = Profiler::_get_memory_usage_(this_process);
                    R ans = std::invoke(std::forward<F>(func), std::forward<Args>(args)...);
                    std::uint64_t ending_memory = Profiler::_get_memory_usage_(this_process);
                    this->profile[Profiler::MEMORY_USAGE] = std::to_string( ending_memory > beginning_memory ?  ending_memory - beginning_memory : beginning_memory - ending_memory );
                    return ans;
                }

                /**
                 * @brief This function measures both execution time and memory usage of a given function. 
                 * 
                 * @tparam R 
                 * @tparam F 
                 * @tparam Args 
                 * @param func 
                 * @param args 
                 * @return R 
                 */
                template <typename R, class F, class ...Args>
                R measure_all(F&& func, Args&&... args) {
                    const pid_t this_process = getpid();
                    std::uint64_t beginning_memory = Profiler::_get_memory_usage_(this_process);
                    this->profile[Profiler::INITIAL_MEMORY] = std::to_string( beginning_memory ); 
                    auto start = ClockT::now();
                    R ans = std::invoke(std::forward<F>(func), std::forward<Args>(args)...);
                    // auto ans = std::invoke(std::forward<F>(func), std::forward<Args>(args)...);
                    this->profile[Profiler::EXECUTION_TIME] = std::to_string( std::chrono::duration_cast<TimeT>(ClockT::now()-start).count() );
                    std::uint64_t ending_memory = Profiler::_get_memory_usage_(this_process);
                    this->profile[Profiler::FINAL_MEMORY] = std::to_string( ending_memory );
                    this->profile[Profiler::MEMORY_USAGE] = std::to_string( ending_memory > beginning_memory ?  ending_memory - beginning_memory : beginning_memory - ending_memory );
                    return ans;
                }

                /**
                 * @brief This function adds a new entry in the measurement profile. 
                 * 
                 * @param key 
                 * @param entry 
                 */
                void add_profile_entry( std::string key, std::string entry ) {
                    this->profile[key] = entry;
                }

                /**
                 * @brief This function returns the measurement profile. 
                 * 
                 * @return std::map<std::string,std::string> 
                 */
                std::map<std::string,std::string> get_profile() {
                    return this->profile;
                }
            private:
                std::map<std::string,std::string> profile;

                /**
                 * @brief This function returns the memory usage of a process. 
                 * 
                 * @param pid 
                 * @return std::uint64_t 
                 */
                static std::uint64_t _get_memory_usage_(pid_t pid) {
                    const std::size_t max_name_length = 64;
                    int fd, data, stack;
                    char buf[4096], status_child[max_name_length];
                    char *vm;

                    sprintf(status_child, "/proc/%d/status", pid);
                    if ((fd = open(status_child, O_RDONLY)) < 0){
                        return -1;
                    }

                    read(fd, buf, 4095);
                    buf[4095] = '\0';
                    close(fd);

                    data = stack = 0;

                    vm = strstr(buf, "VmData:");
                    if (vm) {
                        sscanf(vm, "%*s %d", &data);
                    }
                    vm = strstr(buf, "VmStk:");
                    if (vm) {
                        sscanf(vm, "%*s %d", &stack);
                    }

                    return data + stack;
                }
        };
        template <class TimeT, class ClockT> const std::string Profiler<TimeT,ClockT>::EXECUTION_TIME  = "FUNCTION EXECUTION TIME [ns]";
        template <class TimeT, class ClockT> const std::string Profiler<TimeT,ClockT>::MEMORY_USAGE    = "FUNCTION MEMORY USAGE [B]";
        template <class TimeT, class ClockT> const std::string Profiler<TimeT,ClockT>::INITIAL_MEMORY  = "FUNCTION INITIAL MEMORY [B]";
        template <class TimeT, class ClockT> const std::string Profiler<TimeT,ClockT>::FINAL_MEMORY    = "FUNCTION FINAL MEMORY [B]";
    };
};