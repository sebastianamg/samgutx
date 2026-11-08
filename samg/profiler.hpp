#pragma once
#include <fcntl.h>
#include <unistd.h>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <chrono>
#include <map>
#include <cmath>
#include <string>
#include <functional>

#if defined(__linux__)
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <cstring>
#endif

namespace samg {
    namespace profiler {

        /**
         * @brief This class allows measure exection time and/or memory usage of a given function.
         */
        template <typename TimeT = std::chrono::nanoseconds, typename ClockT = std::chrono::high_resolution_clock>
        class Profiler {
            public:
                /**
                 * @brief A structure to hold the results of a profiling run.
                 * It is designed to be simple and extensible for future metrics.
                 */
                struct ProfileData {
                    std::uint64_t time_ns = 0;
                    std::uint64_t mem_initial_kb = 0;
                    std::uint64_t mem_final_kb = 0;
                    std::uint64_t mem_delta_kb = 0;
                    // Extensible map for future hardware counters (Stage 2)
                    std::map<std::string, std::uint64_t> counters;
                };

                /**
                 * @brief Measures the execution time of a given function.
                 * @tparam R The return type of the function to profile.
                 * @tparam F The function type.
                 * @tparam Args The argument types for the function.
                 * @param func The function to execute and profile.
                 * @param args The arguments to pass to the function.
                 * @return A pair containing the function's return value and the profile data.
                 */
                template <typename R, class F, class ...Args>
                std::pair<R, ProfileData> measure_time(F&& func, Args&&... args) {
                    ProfileData data;
                    auto start = ClockT::now();
                    R ans = std::invoke(std::forward<F>(func), std::forward<Args>(args)...);
                    auto end = ClockT::now();
                    data.time_ns = std::chrono::duration_cast<TimeT>(end - start).count();
                    return {ans, data};
                }

                /**
                 * @brief Measures the memory usage of a given function.
                 * @tparam R The return type of the function to profile.
                 * @tparam F The function type.
                 * @tparam Args The argument types for the function.
                 * @param func The function to execute and profile.
                 * @param args The arguments to pass to the function.
                 * @return A pair containing the function's return value and the profile data.
                 */
                template <typename R, class F, class ...Args>
                std::pair<R, ProfileData> measure_memory(F&& func, Args&&... args) {
                    ProfileData data;
                    const pid_t this_process = getpid();
                    data.mem_initial_kb = Profiler::_get_memory_usage_kb(this_process);
                    R ans = std::invoke(std::forward<F>(func), std::forward<Args>(args)...);
                    data.mem_final_kb = Profiler::_get_memory_usage_kb(this_process);
                    data.mem_delta_kb = (data.mem_final_kb > data.mem_initial_kb) 
                                      ? (data.mem_final_kb - data.mem_initial_kb) 
                                      : 0;
                    return {ans, data};
                }

                /**
                 * @brief Measures both execution time and memory usage of a given function.
                 * @tparam R The return type of the function to profile.
                 * @tparam F The function type.
                 * @tparam Args The argument types for the function.
                 * @param func The function to execute and profile.
                 * @param args The arguments to pass to the function.
                 * @return A pair containing the function's return value and the profile data.
                 */
                template <typename R, class F, class ...Args>
                std::pair<R, ProfileData> measure_all(F&& func, Args&&... args) {
                    ProfileData data;
                    const pid_t this_process = getpid();

                    data.mem_initial_kb = Profiler::_get_memory_usage_kb(this_process);

#if defined(__linux__)
                    constexpr std::uint64_t l1d_read_miss_cfg =
                        PERF_COUNT_HW_CACHE_L1D |
                        (PERF_COUNT_HW_CACHE_OP_READ << 8) |
                        (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
                    constexpr std::uint64_t llc_read_miss_cfg =
                        PERF_COUNT_HW_CACHE_LL |
                        (PERF_COUNT_HW_CACHE_OP_READ << 8) |
                        (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);

                    PerfCounter l1_miss(PERF_TYPE_HW_CACHE, l1d_read_miss_cfg);
                    PerfCounter llc_miss(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES,
                                         PERF_TYPE_HW_CACHE, llc_read_miss_cfg);

                    l1_miss.start();
                    llc_miss.start();
#endif
                    
                    auto start = ClockT::now();
                    R ans = std::invoke(std::forward<F>(func), std::forward<Args>(args)...);
                    auto end = ClockT::now();

#if defined(__linux__)
                    llc_miss.stop();
                    l1_miss.stop();
                    if (l1_miss.valid()) {
                        data.counters["L1_Cache_Misses"] = l1_miss.read();
                    }
                    if (llc_miss.valid()) {
                        data.counters["LLC_Cache_Misses"] = llc_miss.read();
                    }
#endif

                    data.mem_final_kb = Profiler::_get_memory_usage_kb(this_process);
                    data.time_ns = std::chrono::duration_cast<TimeT>(end - start).count();
                    data.mem_delta_kb = (data.mem_final_kb > data.mem_initial_kb) 
                                      ? (data.mem_final_kb - data.mem_initial_kb) 
                                      : 0;

                    return std::make_pair(std::move(ans), data);
                }
            private:
#if defined(__linux__)
                struct PerfCounter {
                    int fd = -1;

                    PerfCounter(std::uint32_t type, std::uint64_t config) {
                        fd = open_event(type, config);
                    }

                    PerfCounter(std::uint32_t primary_type, std::uint64_t primary_config,
                                std::uint32_t fallback_type, std::uint64_t fallback_config) {
                        fd = open_event(primary_type, primary_config);
                        if (fd == -1) {
                            fd = open_event(fallback_type, fallback_config);
                        }
                    }

                    ~PerfCounter() {
                        if (fd != -1) {
                            ::close(fd);
                        }
                    }

                    PerfCounter(const PerfCounter&) = delete;
                    PerfCounter& operator=(const PerfCounter&) = delete;

                    bool valid() const noexcept { return fd != -1; }

                    void start() const noexcept {
                        if (fd != -1) {
                            ::ioctl(fd, PERF_EVENT_IOC_RESET, 0);
                            ::ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
                        }
                    }

                    void stop() const noexcept {
                        if (fd != -1) {
                            ::ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
                        }
                    }

                    std::uint64_t read() const noexcept {
                        std::uint64_t count = 0;
                        if (fd != -1 && ::read(fd, &count, sizeof(count)) == sizeof(count)) {
                            return count;
                        }
                        return 0;
                    }

                private:
                    static int open_event(std::uint32_t type, std::uint64_t config) noexcept {
                        struct perf_event_attr pe{};
                        pe.type = type;
                        pe.size = sizeof(struct perf_event_attr);
                        pe.config = config;
                        pe.disabled = 1;
                        pe.exclude_kernel = 1;
                        pe.exclude_hv = 1;
                        return static_cast<int>(::syscall(__NR_perf_event_open, &pe, 0, -1, -1, 0));
                    }
                };
#endif
                /**
                 * @brief Returns the current memory usage (data + stack) of a process in kilobytes.
                 * @param pid The process ID.
                 * @return Memory usage in KB, or 0 on failure.
                 */
                static std::uint64_t _get_memory_usage_kb(pid_t pid) {
                    std::string status_path = "/proc/" + std::to_string(pid) + "/status";
                    std::ifstream status_file(status_path);
                    if (!status_file.is_open()) {
                        return 0;
                    }

                    std::string line;
                    std::uint64_t data = 0, stack = 0;
                    while (std::getline(status_file, line)) {
                        if (line.rfind("VmData:", 0) == 0) {
                            std::stringstream ss(line);
                            std::string key;
                            ss >> key >> data;
                        } else if (line.rfind("VmStk:", 0) == 0) {
                            std::stringstream ss(line);
                            std::string key;
                            ss >> key >> stack;
                        }
                    }
                    return data + stack;
                }
        };
    };
};