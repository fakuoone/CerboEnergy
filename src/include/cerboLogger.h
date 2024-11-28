#pragma once

#include <string>
#include <vector>
#include <chrono>

#include "imgui.h"

#include "dataTypes.h"

namespace Timing {
    using namespace std::chrono;
    TimingTypes::TimeStruct GetTimeNow() {
        _V2::system_clock::time_point now = system_clock::now();
        milliseconds ms = duration_cast<milliseconds>(now.time_since_epoch());
        time_t nowTimeT = system_clock::to_time_t(now);
        std::tm tm = *std::localtime(&nowTimeT);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%H:%M:%S");                         // Format time as HH:MM:SS
        oss << ":" << std::setfill('0') << std::setw(3) << ms.count(); // Append milliseconds

        return TimingTypes::TimeStruct{ oss.str(), ms.count() };
    }
}

class CerboLog {
private:
    static inline std::vector<LogTypes::Log> logs;
public:
    static uint16_t AddEntry(std::string Text, LogTypes::Categories Category) {
        TimingTypes::TimeStruct now = Timing::GetTimeNow();
        LogTypes::Log log = { now.msString, Text, Category };
        logs.push_back(log);
        return logs.size();
    }

    static uint16_t DisplayLogs() {
        for (int16_t i = logs.size(); i-- > 0;) {
            const char* Cat;
            switch (logs[i].Category) {
            case LogTypes::Categories::SUCCESS:
                Cat = "SUCC";
                break;
            case LogTypes::Categories::FAILURE:
                Cat = "FAIL";
                break;
            case LogTypes::Categories::INFORMATION:
                Cat = "INFO";
                break;
            default:
                break;
            }
            ImGui::Text(logs[i].Timestamp.c_str());
            ImGui::SameLine();
            ImGui::Text(Cat);
            ImGui::SameLine();
            ImGui::TextWrapped(logs[i].Message.c_str());
        }
        return 0;
    }

    static void ResetLogs() {
        logs.clear();
    }
};
