#pragma once

#include "cerboSSH.h"
#include "cerboLogger.h"
#include "cerboPlots.h"
#include "dataHandler.h"
#include "dataTypes.h"

#include "libssh/libssh.h"

#include <stdlib.h>
#include <stdio.h>
#include <string>

class CerboSSH {
    using State = SSHTypes::ConnectionState;

private:
    static inline ssh_session currentSession = NULL;
    static inline ssh_channel currentChannel = NULL;
    static inline std::string ipAdress{ "192.168.178.110" };
    static inline std::string command{ "cat /data/home/nodered/energyLog.csv" };
    static inline std::string logString{ "" };
    static inline std::string commandResult{ "" };
    static inline int16_t port{ 22 };
    static inline State connectionState{ State::OFFLINE };

    struct Login {
        std::string userName;
        std::string password;
    };


    static void VerifyConnectionState() {
        if (currentSession == NULL) {
            connectionState = State::OFFLINE;
            return;
        } else {
            connectionState == State::SESSION;
        }
        if (!ssh_is_connected(currentSession)) {
            connectionState = State::SESSION;
        }
        // NO FEEDBACK IN TERMS OF AUTHENTICATION HERE
        if (currentChannel == NULL) {
            if (connectionState > State::AUTHENTICATED) {
                connectionState = State::AUTHENTICATED;
            }
            return;
        }
        // NO FEEDBACK IN TERMS OF CHANNEL SESSION HERE
        // NO FEEDBACK IN TERMS OF EXECUTED CMD HERE
        if (commandResult != "") {
            connectionState = State::READ_RESULT;
        }
    }

    static bool CheckConnectionState(State toCheck, SSHTypes::ComparisonType compType) {
        VerifyConnectionState();
        switch (compType) {
        case SSHTypes::ComparisonType::GR:
            return connectionState > toCheck;
            break;
        case SSHTypes::ComparisonType::GREQ:
            return connectionState >= toCheck;
            break;
        case SSHTypes::ComparisonType::EQ:
            return connectionState == toCheck;
            break;
        case SSHTypes::ComparisonType::INEQ:
            return connectionState != toCheck;
            break;
        case SSHTypes::ComparisonType::SMEQ:
            return connectionState <= toCheck;
            break;
        case SSHTypes::ComparisonType::SM:
            return connectionState < toCheck;
            break;
        default:
            return false;
            break;
        }
    }

    static uint16_t CreateSession() {
        if (CheckConnectionState(State::OFFLINE, SSHTypes::ComparisonType::INEQ)) {
            logString = "Failed to create SSH-Session. Connection-state is not offline.";
            CerboLog::AddEntry(logString, LogTypes::Categories::FAILURE);
            return SSH_ERROR;
        }
        logString = "Session created";
        CerboLog::AddEntry(logString, LogTypes::Categories::SUCCESS);
        currentSession = ssh_new();
        if (currentSession == NULL) {
            logString = "Failed to create SSH-Session. No connection possible.";
            CerboLog::AddEntry(logString, LogTypes::Categories::FAILURE);
            return SSH_ERROR;
        }
        ssh_options_set(currentSession, SSH_OPTIONS_HOST, ipAdress.c_str());
        ssh_options_set(currentSession, SSH_OPTIONS_PORT, &port);
        connectionState = State::SESSION;
        return SSH_OK;

    }

    static Login ReadPassword() {
        Login login;
        std::string filename = "B:/Programmieren/C/CerboEnergy/doc/passwordSSH.txt";
        std::ifstream loginStream(filename);
        std::string temp;
        if (!loginStream.is_open()) {
            std::string errorText = "Can't open file" + filename;
            CerboLog::AddEntry(errorText, LogTypes::Categories::FAILURE);
        } else {
            while (getline(loginStream, temp)) {
                login.userName = temp;
            }
            while (getline(loginStream, temp)) {
                login.password = temp;
            }
        }
        return login;
    }

    static uint16_t AuthenticateConnection() {
        if (CheckConnectionState(State::SESSION, SSHTypes::ComparisonType::INEQ)) {
            logString = "Failed to authenticate. No session going on.";
            CerboLog::AddEntry(logString, LogTypes::Categories::FAILURE);
            return SSH_ERROR;
        }
        int16_t connection = ssh_connect(currentSession);
        if (connection != SSH_OK) {
            logString = "Can't connect to " + ipAdress + ": " + std::string(ssh_get_error(currentSession));
            CerboLog::AddEntry(logString, LogTypes::Categories::FAILURE);
            return SSH_ERROR;
        }
        logString = "Connected to: " + ipAdress;
        CerboLog::AddEntry(logString, LogTypes::Categories::SUCCESS);
        Login sshLogin = ReadPassword();
        int16_t setPassword = ssh_userauth_password(currentSession, sshLogin.userName.c_str(), sshLogin.password.c_str());
        if (setPassword != SSH_AUTH_SUCCESS) {
            logString = "Error authenticating with password: " + std::string(ssh_get_error(currentSession));
            CerboLog::AddEntry(logString, LogTypes::Categories::FAILURE);
            return SSH_ERROR;
        }
        logString = "Successfully authenticated.";
        CerboLog::AddEntry(logString, LogTypes::Categories::SUCCESS);
        connectionState = State::AUTHENTICATED;
        return SSH_OK;
    }

    static uint16_t CreateChannelWithSession() {
        if (CheckConnectionState(State::AUTHENTICATED, SSHTypes::ComparisonType::INEQ)) {
            logString = "Failed to create channel. No authenticated session going on.";
            CerboLog::AddEntry(logString, LogTypes::Categories::FAILURE);
            return SSH_ERROR;
        }
        currentChannel = ssh_channel_new(currentSession);
        if (currentChannel == NULL) {
            logString = "Failed to open channel: " + std::string(ssh_get_error(currentSession));
            CerboLog::AddEntry(logString, LogTypes::Categories::FAILURE);
            return SSH_ERROR;
        }
        logString = "Successfully created channel.";
        CerboLog::AddEntry(logString, LogTypes::Categories::SUCCESS);
        connectionState = State::CHANNEL;

        if (ssh_channel_open_session(currentChannel) != SSH_OK) {
            logString = "Failed to open session on channel: " + std::string(ssh_get_error(currentSession));
            CerboLog::AddEntry(logString, LogTypes::Categories::FAILURE);
            ssh_channel_free(currentChannel);
            currentChannel = NULL;
            return SSH_ERROR;
        }
        logString = "Successfully created session on channel.";
        CerboLog::AddEntry(logString, LogTypes::Categories::SUCCESS);
        connectionState = State::CHANNEL_SESSION;
        return SSH_OK;
    }

    static uint16_t ExecuteRemoteCommand() {
        if (CheckConnectionState(State::CHANNEL_SESSION, SSHTypes::ComparisonType::INEQ)) {
            logString = "Can't execute command, no authenticated session with channel going on.";
            CerboLog::AddEntry(logString, LogTypes::Categories::FAILURE);
            return SSH_ERROR;
        }
        if (ssh_channel_request_exec(currentChannel, command.c_str()) != SSH_OK) {
            logString = "Error executing command " + command + ": " + std::string(ssh_get_error(currentSession));
            CerboLog::AddEntry(logString, LogTypes::Categories::FAILURE);
            return SSH_ERROR;
        }
        logString = "Successfully executed command: " + command;
        CerboLog::AddEntry(logString, LogTypes::Categories::SUCCESS);
        connectionState = State::EXECUTED_CMD;
        return SSH_OK;
    }

    static uint16_t ReadData() {
        if (CheckConnectionState(State::EXECUTED_CMD, SSHTypes::ComparisonType::INEQ)) {
            logString = "Can't read data, no command was executed.";
            CerboLog::AddEntry(logString, LogTypes::Categories::FAILURE);
            return SSH_ERROR;
        }
        int16_t nbytes;
        commandResult = "";
        while (ssh_channel_is_open(currentChannel) && !ssh_channel_is_eof(currentChannel)) {
            char buffer[128];
            nbytes = ssh_channel_read(currentChannel, buffer, sizeof(buffer), 0);
            if (nbytes < 0) {
                logString = "Failed to read from channel: " + std::string(ssh_get_error(currentSession));
                CerboLog::AddEntry(logString, LogTypes::Categories::SUCCESS);
                return SSH_ERROR;
            }
            commandResult.append(buffer, nbytes);
        }

        logString = "Successfully read " + std::to_string(commandResult.length()) + " bytes of data:\n" + commandResult.substr(0, 100);
        if (commandResult.length() > 100) {
            logString += "...";
        }
        CerboLog::AddEntry(logString, LogTypes::Categories::SUCCESS);
        connectionState = State::READ_RESULT;
        return SSH_OK;
    }

    static uint16_t EndChannel() {
        if (CheckConnectionState(State::CHANNEL, SSHTypes::ComparisonType::SM)) {
            logString = "No Channel exists. Nothing to terminate.";
            CerboLog::AddEntry(logString, LogTypes::Categories::INFORMATION);
            return SSH_ERROR;
        }
        ssh_channel_close(currentChannel);
        ssh_channel_free(currentChannel);
        currentChannel = NULL;
        logString = "Channel terminated.";
        CerboLog::AddEntry(logString, LogTypes::Categories::SUCCESS);
        connectionState = State::AUTHENTICATED;
        return SSH_OK;
    }

    static uint16_t EndSession() {
        if (CheckConnectionState(State::SESSION, SSHTypes::ComparisonType::SM)) {
            logString = "No Session exists. Nothing to terminate.";
            CerboLog::AddEntry(logString, LogTypes::Categories::INFORMATION);
            return SSH_ERROR;
        }
        ssh_free(currentSession);
        currentSession = NULL;
        commandResult = "";
        connectionState = State::OFFLINE;
        logString = "Session terminated.";
        CerboLog::AddEntry(logString, LogTypes::Categories::SUCCESS);
        return SSH_OK;
    }


public:
    static uint16_t ConnectCerbo() {
        if (CheckConnectionState(State::CHANNEL, SSHTypes::ComparisonType::GREQ)) {
            EndChannel();
        } else {
            if (CreateSession() != SSH_OK)
                return SSH_ERROR;
            if (AuthenticateConnection() != SSH_OK)
                return SSH_ERROR;
            if (CreateChannelWithSession() != SSH_OK)
                return SSH_ERROR;
        }
        return SSH_OK;
    }

    static uint16_t DisconnectCerbo() {
        EndChannel();
        if (CheckConnectionState(State::CONNECTED, SSHTypes::ComparisonType::GREQ)) {
            ssh_disconnect(currentSession);
            logString = "Disconnected from: " + ipAdress;
            CerboLog::AddEntry(logString, LogTypes::Categories::SUCCESS);
        }
        EndSession();
        return SSH_OK;
    }

    static uint16_t ReadEnergyFile() {
        if (CheckConnectionState(State::CONNECTED, SSHTypes::ComparisonType::SM)) {
            ConnectCerbo();
        }
        if (!SSHDataHandler::ConversionPending()) {
            if (ExecuteRemoteCommand() != SSH_OK)
                return SSH_ERROR;
            if (ReadData() != SSH_OK)
                return SSH_ERROR;
        }
        return SSH_OK;
    }

    static std::string GetRawString() {
        return commandResult;
    }

    static State GetConnectionState() {
        return connectionState;
    }
};