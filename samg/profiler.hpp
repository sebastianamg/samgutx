#pragma once
#include <chrono>
#include <map>
#include <string>
#include <unistd.h>
namespace samg {
    namespace profiler {
        template <class TimeT  = std::chrono::nanoseconds, class ClockT = std::chrono::steady_clock>
        class Profiler {
            public:
                enum ProfileEntry {
                        EXECUTION_TIME,
                        MEMORY_USAGE
                };
                
                template <class F, class ...Args>
                auto measure_time(F&& func, Args&&... args) {
                    auto start = ClockT::now();
                    auto ans = std::invoke(std::forward<F>(func), std::forward<Args>(args)...);
                    this->profile[ProfileEntry::EXECUTION_TIME] = std::chrono::duration_cast<TimeT>(ClockT::now()-start).count();
                    return ans;
                }

                template <class F, class ...Args>
                auto measure_memory(F&& func, Args&&... args) {
                    const pid_t this_process = getpid();
                    std::uint64_t beginning_memory = _get_memory_usage_(this_process);
                    auto ans = std::invoke(std::forward<F>(func), std::forward<Args>(args)...);
                    std::uint64_t ending_memory = _get_memory_usage_(this_process);
                    this->profile[ProfileEntry::MEMORY_USAGE] = ending_memory > beginning_memory ?  ending_memory - beginning_memory : beginning_memory - ending_memory;
                    return ans;
                }

                template <class F, class ...Args>
                auto measure_all(F&& func, Args&&... args) {
                    const pid_t this_process = getpid();
                    std::uint64_t beginning_memory = _get_memory_usage_(this_process);
                    auto start = ClockT::now();
                    auto ans = std::invoke(std::forward<F>(func), std::forward<Args>(args)...);
                    this->profile[ProfileEntry::EXECUTION_TIME] = std::chrono::duration_cast<TimeT>(ClockT::now()-start).count();
                    std::uint64_t ending_memory = _get_memory_usage_(this_process);
                    this->profile[ProfileEntry::MEMORY_USAGE] = ending_memory > beginning_memory ?  ending_memory - beginning_memory : beginning_memory - ending_memory;
                    return ans;
                }

                std::map<ProfileEntry,double> get_profile() {
                    return this->profile;
                }
            private:
                std::map<ProfileEntry,double> profile;

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
    };
};