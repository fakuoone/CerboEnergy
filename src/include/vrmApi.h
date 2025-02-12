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

    static inline ApiTypes::ConnectionData conn;

    static ApiTypes::LoginData ReadUserValues() {
        ApiTypes::LoginData conn;
        std::string filename = "B:/Programmieren/C/CerboEnergy/doc/passwordApi.txt";
        std::ifstream loginStream(filename);
        std::string temp;
        if (!loginStream.is_open()) {
            std::string errorText = "Can't open file" + filename;
            CerboLog::AddEntry(errorText, LogTypes::Categories::FAILURE);
            return conn;
        }
        if (!std::getline(loginStream, conn.userName)) {
            CerboLog::AddEntry("Username line missing in file.", LogTypes::Categories::FAILURE);
        }
        if (!std::getline(loginStream, conn.password)) {
            CerboLog::AddEntry("Password line missing in file.", LogTypes::Categories::FAILURE);
        }
        if (!std::getline(loginStream, temp)) {
            CerboLog::AddEntry("SiteID line missing in file.", LogTypes::Categories::FAILURE);
        }
        conn.siteID = std::stoi(temp);
        return conn;
    }

    static std::string formatUrl(TimingTypes::TimeStruct start, TimingTypes::TimeStruct end) {
        std::string returnString = "https://vrmapi.victronenergy.com/v2/" + std::to_string(conn.login.siteID) + "installations/237026/stats?";
        returnString += "start=" + std::to_string(static_cast<int32_t>(start.ms / 1000)) + "&end=" + std::to_string(static_cast<int32_t>(end.ms / 1000)) + "&interval=days";
        return returnString;
    }

    static void ParseData() {
        conn.responseJson = nlohmann::json::parse(conn.responseString);
        if (!ReadResponseInfo()) {
            CerboLog::AddEntry("API responded with error.", LogTypes::Categories::FAILURE);
            return;
        }
        switch (conn.state) {
            case ApiTypes::ConnectionState::AUTHENTICATING:
                if (ReadToken()) {
                    conn.state = ApiTypes::ConnectionState::AUTHENTICATED;
                }
                break;
            case ApiTypes::ConnectionState::REQUESTING:
                if (ReadData()) {
                    conn.state = ApiTypes::ConnectionState::DATA_RECEIVED;
                }
                break;
            default:
                break;
        }
    }

    static bool ReadResponseInfo() {
        if (conn.responseJson.contains("success")) {
            conn.responseInfo.success = conn.responseJson["success"];
        }
        if (conn.responseJson.contains("errors")) {
            conn.responseInfo.errors = conn.responseJson["errors"];
        }
        if (conn.responseJson.contains("errorCode")) {
            conn.responseInfo.errorCode = conn.responseJson["errorCode"];
        }
        return conn.responseInfo.success;
    }

    static bool ReadToken() {
        CerboLog::AddEntry(conn.responseString, LogTypes::Categories::INFORMATION);
        conn.responseJson = nlohmann::json::parse(conn.responseString);
        if (conn.responseJson.contains("token")) {
            conn.login.token = conn.responseJson["token"];
            return true;
        }
        return false;
    }

    static bool ReadData() { return false; }

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
        size_t totalSize = size * nmemb;
        s->append(static_cast<char*>(contents), totalSize);
        ParseData();
        return totalSize;
    }

   public:
    static void Connect() {
        curl = curl_easy_init();
        if (curl) {
            conn.login = ReadUserValues();

            conn.responseString.clear();
            conn.responseJson.clear();

            conn.state = ApiTypes::ConnectionState::AUTHENTICATING;
            curl_easy_setopt(curl, CURLOPT_URL, "https://vrmapi.victronenergy.com/v2/auth/conn");
            curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_3);

            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &conn.responseString);

            struct curl_slist* headers = NULL;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            std::string payload = "{\n  \"username\": \"" + conn.login.userName + "\",\n  \"password\": \"" + conn.login.password + "\"\n}";
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());

            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                CerboLog::AddEntry("Error loggin in.", LogTypes::Categories::FAILURE);
                curl_easy_cleanup(curl);
                return;
            }
            conn.state = ApiTypes::ConnectionState::AUTHENTICATED;
            CerboLog::AddEntry("Successfully logged in.", LogTypes::Categories::SUCCESS);
        }
    }

    static void GetData(TimingTypes::TimeStruct start, TimingTypes::TimeStruct end) {
        conn.responseString.clear();
        conn.responseJson.clear();

        conn.state = ApiTypes::ConnectionState::REQUESTING;
        const std::string url = formatUrl(start, end);
        const std::string messageString = "Attempting to connect via: " + url;
        CerboLog::AddEntry(messageString, LogTypes::Categories::INFORMATION);

        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        const std::string headerString = "x-authorization: Bearer " + conn.login.token;
        headers = curl_slist_append(headers, headerString.c_str());

        CURLcode ret = curl_easy_perform(curl);
        if (ret != CURLE_OK) {
            CerboLog::AddEntry("Error getting data.", LogTypes::Categories::FAILURE);
            return;
        }
        conn.state = ApiTypes::ConnectionState::DATA_RECEIVED;
    }

    static ApiTypes::ConnectionState GetConnectionState() { return conn.state; }

    static void Disconnect() {
        conn.responseString.clear();
        conn.responseJson.clear();

        curl_easy_cleanup(curl);
        conn.state = ApiTypes::ConnectionState::OFFLINE;
    }
};
