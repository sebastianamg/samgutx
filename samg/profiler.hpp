#include <chrono>
#include <map>
#include <string>
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
                std::map<ProfileEntry,double> get_profile() {
                    return this->profile;
                }
            private:
                std::map<ProfileEntry,double> profile;
        };
    };
};