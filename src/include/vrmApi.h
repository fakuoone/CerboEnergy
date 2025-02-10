#pragma once

#include "cerboLogger.h"

#include <curl/curl.h>

#include <fstream>
#include <string>

class CerboVrm {
   private:
    static inline int16_t a = 0;
    static inline CURL* curl;
    static inline CURLcode res;
    static inline std::string responseString;

    struct UserValues {
        std::string userName;
        std::string password;
        int64_t siteID;
    };

    static inline UserValues login;

    static UserValues ReadUserValues() {
        UserValues login;
        std::string filename = "B:/Programmieren/C/CerboEnergy/doc/passwordApi.txt";
        std::ifstream loginStream(filename);
        std::string temp;
        if (!loginStream.is_open()) {
            std::string errorText = "Can't open file" + filename;
            CerboLog::AddEntry(errorText, LogTypes::Categories::FAILURE);
            return login;
        }
        if (!std::getline(loginStream, login.userName)) {
            CerboLog::AddEntry("Username line missing in file.", LogTypes::Categories::FAILURE);
        }
        if (!std::getline(loginStream, login.password)) {
            CerboLog::AddEntry("Password line missing in file.", LogTypes::Categories::FAILURE);
        }
        if (!std::getline(loginStream, temp)) {
            CerboLog::AddEntry("SiteID line missing in file.", LogTypes::Categories::FAILURE);
        }
        login.siteID = std::stoi(temp);
        return login;
    }

    static std::string formatUrl(TimingTypes::TimeStruct start, TimingTypes::TimeStruct end) {
        std::string returnString = "https://vrmapi.victronenergy.com/v2/" + std::to_string(login.siteID) + "installations/237026/stats?";
        returnString += "start=" + std::to_string(static_cast<int32_t>(start.ms / 1000)) + "&end=" + std::to_string(static_cast<int32_t>(end.ms / 1000)) + "&interval=days";
        return returnString;
    }

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
        size_t totalSize = size * nmemb;
        s->append(static_cast<char*>(contents), totalSize);
        return totalSize;
        // TBD: Abhängig vom aktuellen Zustand muss hier der responseString ausgewertet werden
        // 1. TOKEN speichern und für spätere Anfragen verwenden
    }

   public:
    static void Connect() {
        curl = curl_easy_init();
        if (curl) {
            login = ReadUserValues();
            curl_easy_setopt(curl, CURLOPT_URL, "https://vrmapi.victronenergy.com/v2/auth/login");
            curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_3);

            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

            struct curl_slist* headers = NULL;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            std::string payload = "{\n  \"username\": \"" + login.userName + "\",\n  \"password\": \"" + login.password + "\"\n}";
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());

            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                CerboLog::AddEntry("Error loggin in.", LogTypes::Categories::FAILURE);
                curl_easy_cleanup(curl);
                return;
            }
            CerboLog::AddEntry("Successfully logged in.", LogTypes::Categories::SUCCESS);
        }
    }

    static void GetData(TimingTypes::TimeStruct start, TimingTypes::TimeStruct end) {
        const std::string url = formatUrl(start, end);
        const std::string messageString = "Attempting to connect via: " + url;
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        CerboLog::AddEntry(messageString, LogTypes::Categories::INFORMATION);
        CURLcode ret = curl_easy_perform(curl);
    }

    static void Disconnect() { curl_easy_cleanup(curl); }
};
