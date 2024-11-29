#pragma once

#include "cerboPlots.h"
#include "cerboLogger.h"
#include "dataTypes.h"

#include <string>
#include <sstream>
#include <unordered_map>
#include <map>
#include <numeric>
#include <algorithm>

#define PARSING_CHUNKSIZE 200

class SSHDataHandler {
private:
    inline static PDTypes::HelperData CD{ 200 };
    inline static std::map<uint16_t, std::string> KeyLookup;
    inline static std::map<std::string, std::string> KeyNameLookup = {
        {"timestamp", "Zeitstempel"},
        {"wBatnegative", "Entladeenergie"},
        {"wBatpositive", "Ladeenergie"},
        {"wSolnegative", "Solarkonsum"},
        {"wSolpositive", "Solarerzeugung"},
        {"wGridnegative", "Einspeiseenergie"},
        {"wGridpositive", "Netzbezug"},
        {"wLoadnegative", "AC-Erzeuger"},
        {"wLoadpositive", "Lasten"} };

public:
    static void PHead(PDTypes::EnergyStruct& ED, std::string rawData) {
        using namespace std;
        vector<float> emptyVector;

        CD.HeaderEnd = CD.Progress = rawData.find("\n");
        CD.DataSize = rawData.length();
        string temp = rawData.substr(0, CD.HeaderEnd);
        stringstream ss(temp);
        uint16_t i = 0;

        while (getline(ss, temp, ',')) {
            PDTypes::Entries firstEntry = PDTypes::Entries();
            if (temp != "timestamp") {
                ED.Daily.Es.insert({ temp, firstEntry });
                KeyLookup.insert({ i, temp });
                i++;
            }
            CD.Header.push_back(temp);
        }

        CD.DataCategories = i;
        string logMessage = "Header parsed, size: " + to_string(CD.HeaderEnd) + " bytes.";
        CerboLog::AddEntry(logMessage, LogTypes::Categories::SUCCESS);
    }

    static bool FormatData(PDTypes::EnergyStruct& ED, std::string rawData) {
        using namespace std;
        vector<float> emptyVector;

        if (CD.Progress < CD.HeaderEnd)
            PHead(ED, rawData);

        uint16_t PreviousProgress = CD.Progress;
        bool KeepConverting = CD.Progress != CD.DataSize;

        while (KeepConverting) {
            uint64_t NewRowEnd = rawData.find("\n", ++CD.Progress);
            if (NewRowEnd == string::npos)
                NewRowEnd = CD.DataSize;

            string temp = rawData.substr(CD.Progress, NewRowEnd - CD.Progress);
            stringstream ss(temp);
            float tempData = 0.0f;
            uint16_t i = 0;
            uint32_t tempTimestamp = 0;
            uint64_t veryLong = 0;

            while (getline(ss, temp, ',')) {
                if (i == 0) // Äquivalent zu if i == 0, da der erste Eintrag timestamp sein MUSS
                {
                    try {
                        veryLong = std::stoll(temp) / 1000;
                        tempTimestamp = veryLong;
                    } catch (std::exception& e) {
                        string errorText = "Can't convert timestamp to integer: " + string(e.what());
                        CD.Error = true;
                        CerboLog::AddEntry(errorText, LogTypes::Categories::FAILURE);
                        return false;
                    }
                    ED.Daily.Times.push_back(tempTimestamp);
                } else {
                    try {
                        tempData = stof(temp);
                    } catch (const std::exception& e) {
                        string errorText = "Can't convert to float: " + string(e.what());
                        CD.Error = true;
                        CerboLog::AddEntry(errorText, LogTypes::Categories::FAILURE);
                        return false;
                    }
                    ED.Daily.Es[KeyLookup[i - 1]].Values.push_back(tempData);
                }
                i++;
            }

            CD.RowsConverted++;
            CD.Progress += (NewRowEnd - CD.Progress);
            CD.AvgRowSize = (CD.Progress - CD.HeaderEnd) / CD.RowsConverted;

            if ((CD.Progress - PreviousProgress + CD.AvgRowSize / 2) > CD.TargetChunkSize)
                KeepConverting = false;

            if (CD.Progress == CD.DataSize) {
                KeepConverting = false;
                CD.Completed = true;
                CD.Error = false;
            }
        }
        string logText = "Parsed " + to_string(CD.Progress - PreviousProgress) + " bytes of data.";
        CerboLog::AddEntry(logText, LogTypes::Categories::SUCCESS);

        return CD.Completed;
    }

    static void ResetSSHData() {
        CD = PDTypes::HelperData{ 200 };
    }

    static std::vector<std::string> GetHeader() {
        return CD.Header;
    }

    static void ComputeAnalytics(PDTypes::EnergyStruct& ED) {
        for (int16_t i = 0; i < ED.Daily.Es.size(); i++) {
            using fIter = std::vector<float>::iterator;
            PDTypes::Entries& refEntry = ED.Daily.Es[KeyLookup[i]];
            fIter begin = refEntry.Values.begin();
            fIter end = refEntry.Values.end();
            std::pair<fIter, fIter> minmax = std::minmax_element(begin, end);
            refEntry.AN.Min.Value = *minmax.first;
            refEntry.AN.Max.Value = *minmax.second;
            refEntry.AN.Min.Timestamp = ED.Daily.Times[minmax.first - begin];
            refEntry.AN.Max.Timestamp = ED.Daily.Times[minmax.second - begin];
            refEntry.AN.sum = std::accumulate(begin, end, 0.0f);
        }
    }

    static bool ConversionPending() {
        return CD.Progress != 0 && !CD.Completed && !CD.Error;
    }

    static bool DataAvailable() {
        return CD.Completed;
    }

    static float GetConversionProgress() {
        float progress = CD.DataSize != 0 ? (float)CD.Progress / (float)CD.DataSize : 0;
        if (progress > 0) {
            int16_t a = 0;
        }
        return progress;
    }

    static std::string GetPlotKey(uint16_t iBar) {
        return KeyLookup[iBar];
    }

    static std::string GetPlotName(uint16_t iBar) {
        return KeyNameLookup[GetPlotKey(iBar)];
    }

    static std::string GetTopicName(uint16_t i) {
        return KeyNameLookup[KeyLookup[i]];
    }
    static uint16_t GetDataCategories() {
        return CD.DataCategories;
    }
};

class APIDataHandler {
private:
    int16_t a = 0;
public:
    void foo() {

    }
};

class RTDataHandler {
private:
    int16_t a = 0;
    int64_t fun = 0;
public:
    void foo() {

    }
};