#pragma once

#include <cassert>
#include <chrono>
#include <string>
#include <vector>

#include "imgui.h"

#include "dataTypes.h"

namespace Timing {
using namespace std::chrono;
TimingTypes::TimeStruct GetTimeNow() {  // Timestamp has bugs
    _V2::system_clock::time_point now = system_clock::now();
    milliseconds ms = duration_cast<milliseconds>(now.time_since_epoch());
    time_t nowTimeT = system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&nowTimeT);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");                                // Format time as HH:MM:SS
    oss << ":" << std::setfill('0') << std::setw(3) << ms.count() % 100;  // Append milliseconds

    return TimingTypes::TimeStruct{oss.str(), ms.count()};
}

TimingTypes::TimeStruct GetTimeFromSeconds(int32_t seconds) {
    time_t nowTimeT = static_cast<time_t>(seconds);
    std::tm tm = *std::localtime(&nowTimeT);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");            // Format time as HH:MM:SS
    oss << ":" << std::setfill('0') << std::setw(3);  // Append milliseconds

    int64_t ms = static_cast<int64_t>(seconds) * 1000;

    return TimingTypes::TimeStruct{oss.str(), ms};
}
}  // namespace Timing

class CerboLog {
   private:
    static inline std::vector<LogTypes::Log> logs;

   public:
    static void AddEntry(std::string Text, LogTypes::Categories Category) {
        const TimingTypes::TimeStruct now = Timing::GetTimeNow();
        logs.push_back(LogTypes::Log{now.msString, std::move(Text), Category});
    }

    static std::vector<LogTypes::Log>& GetLogs() { return logs; }

    static void ResetLogs() { logs.clear(); }
};
