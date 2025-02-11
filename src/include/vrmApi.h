#pragma once

#include "cerboLogger.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <fstream>
#include <string>

class CerboVrm {
   private:
    static inline int16_t a = 0;
    static inline CURL* curl;
    static inline CURLcode res;
    static inline std::string responseString;
    static inline nlohmann::json responseJson;

    static inline ApiTypes::UserValues login;
    static inline ApiTypes::ConnectionState connectionState{ApiTypes::ConnectionState::OFFLINE};

    static ApiTypes::UserValues ReadUserValues() {
        ApiTypes::UserValues login;
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

    static bool ParseToken() {
        responseJson = nlohmann::json::parse(responseString);
        if (responseJson.contains("token")) {
            login.token = responseJson["token"];
            return true;
        }
        return false;
    }

    static bool ParseData() {
        responseJson = nlohmann::json::parse(responseString);
        return false;
    }

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
        size_t totalSize = size * nmemb;
        s->append(static_cast<char*>(contents), totalSize);
        switch (connectionState) {
            case ApiTypes::ConnectionState::AUTHENTICATING:
                if (ParseToken()) {
                    connectionState = ApiTypes::ConnectionState::AUTHENTICATED;
                }
                break;
            case ApiTypes::ConnectionState::REQUESTING:
                if (ParseData()) {
                    connectionState = ApiTypes::ConnectionState::DATA_RECEIVED;
                }
                break;
            default:
                break;
        }
        return totalSize;
    }

   public:
    static void Connect() {
        curl = curl_easy_init();
        if (curl) {
            login = ReadUserValues();
            connectionState = ApiTypes::ConnectionState::AUTHENTICATING;
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
            connectionState = ApiTypes::ConnectionState::AUTHENTICATED;
            CerboLog::AddEntry("Successfully logged in.", LogTypes::Categories::SUCCESS);
        }
    }

    static void GetData(TimingTypes::TimeStruct start, TimingTypes::TimeStruct end) {
        connectionState = ApiTypes::ConnectionState::REQUESTING;
        const std::string url = formatUrl(start, end);
        const std::string messageString = "Attempting to connect via: " + url;
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        CerboLog::AddEntry(messageString, LogTypes::Categories::INFORMATION);
        CURLcode ret = curl_easy_perform(curl);
        if (ret != CURLE_OK) {
            CerboLog::AddEntry("Error getting data.", LogTypes::Categories::FAILURE);
            return;
        }
        connectionState = ApiTypes::ConnectionState::DATA_RECEIVED;
    }

    static ApiTypes::ConnectionState GetConnectionState() { return connectionState; }

    static void Disconnect() {
        curl_easy_cleanup(curl);
        connectionState = ApiTypes::ConnectionState::OFFLINE;
    }
};
