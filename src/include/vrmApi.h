#pragma once

#include "cerboLogger.h"

#include <curl/curl.h>

#include <fstream>
#include <string>

class CerboVrm
{
  private:
    static inline int16_t a = 0;
    static inline CURL* curl;
    static inline CURLcode res;

    struct Login
    {
        std::string userName;
        std::string password;
    };

    static Login ReadPassword()
    {
        Login login;
        std::string filename = "B:/Programmieren/C/CerboEnergy/doc/passwordApi.txt";
        std::ifstream loginStream(filename);
        if (!loginStream.is_open())
        {
            std::string errorText = "Can't open file" + filename;
            CerboLog::AddEntry(errorText, LogTypes::Categories::FAILURE);
            return login;
        }
        if (std::getline(loginStream, login.userName))
        {
            if (!std::getline(loginStream, login.password))
            {
                CerboLog::AddEntry("Password line missing in file.", LogTypes::Categories::FAILURE);
            }
        }
        else
        {
            CerboLog::AddEntry("Username line missing in file.", LogTypes::Categories::FAILURE);
        }
        return login;
    }

  public:
    static void Connect()
    {
        curl = curl_easy_init();
        if (curl)
        {
            curl_easy_setopt(curl, CURLOPT_URL, "https://vrmapi.victronenergy.com/v2/auth/login");
            curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_3);
            struct curl_slist* headers = NULL;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            Login login = ReadPassword();
            std::string payload =
                "{\n  \"username\": \"" + login.userName + "\",\n  \"password\": \"" + login.password + "\"\n}";
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());

            res = curl_easy_perform(curl);
            if (res != CURLE_OK)
            {
                CerboLog::AddEntry("Error loggin in.", LogTypes::Categories::FAILURE);
                curl_easy_cleanup(curl);
            }
            CerboLog::AddEntry("Successfully logged in.", LogTypes::Categories::SUCCESS);
            curl_easy_cleanup(curl);
        }
    }

    static void GetData() {}
};
