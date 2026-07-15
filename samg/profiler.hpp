#pragma once
#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include <chrono>
#include <map>
#include <cmath>
#include <string>
#include <functional>

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
                    
                    auto start = ClockT::now();
                    R ans = std::invoke(std::forward<F>(func), std::forward<Args>(args)...);
                    auto end = ClockT::now();

                    data.mem_final_kb = Profiler::_get_memory_usage_kb(this_process);
                    data.time_ns = std::chrono::duration_cast<TimeT>(end - start).count();
                    data.mem_delta_kb = (data.mem_final_kb > data.mem_initial_kb) 
                                      ? (data.mem_final_kb - data.mem_initial_kb) 
                                      : 0;

                    return std::make_pair(std::move(ans), data);
                }
            private:
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