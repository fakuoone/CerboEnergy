#pragma once

#include <cassert>
#include <chrono>
#include <string>
#include <vector>

#include "imgui.h"

#include "dataTypes.h"

namespace Timing
{
using namespace std::chrono;
TimingTypes::TimeStruct GetTimeNow()
{
    _V2::system_clock::time_point now = system_clock::now();
    milliseconds ms = duration_cast<milliseconds>(now.time_since_epoch());
    time_t nowTimeT = system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&nowTimeT);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");                         // Format time as HH:MM:SS
    oss << ":" << std::setfill('0') << std::setw(3) << ms.count(); // Append milliseconds

    return TimingTypes::TimeStruct{oss.str(), ms.count()};
}
} // namespace Timing

class CerboLog
{
  private:
    static inline std::vector<LogTypes::Log> logs;

  public:
    static void AddEntry(std::string Text, LogTypes::Categories Category)
    {
        const TimingTypes::TimeStruct now = Timing::GetTimeNow();
        logs.push_back(LogTypes::Log{now.msString, std::move(Text), Category});
    }

    static void DisplayLogs()
    {
        for (auto it = logs.crbegin(); it != logs.crend(); ++it)
        {
            const auto current_log = *it;

            ImGui::Text(current_log.Timestamp.c_str());
            ImGui::SameLine();

            if (current_log.Category == LogTypes::Categories::SUCCESS)
            {
                ImGui::Text("SUCC");
            }
            else if (current_log.Category == LogTypes::Categories::FAILURE)
            {
                ImGui::Text("FAIL");
            }
            else
            {
                assert(current_log.Category == LogTypes::Categories::INFORMATION);
                ImGui::Text("INFO");
            }
            ImGui::SameLine();
            ImGui::TextWrapped(current_log.Message.c_str());
        }
    }

    static void ResetLogs() { logs.clear(); }
};
