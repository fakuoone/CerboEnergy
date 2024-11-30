#pragma once

#include <string>

#include <modbus/modbus.h>

#include "cerboLogger.h"
#include "dataTypes.h"

using namespace ModbusTypes;
class Register
{
  private:
    bool ToBeRead;
    bool ToBeSaved{true};
    int64_t TimeDeltaBetweenReads;
    RegisterResult Result;
    ModbusTypes::CircularBuffer<double, CIRC_BUFFER_SIZE> ValueStorage;
    ModbusTypes::CircularBuffer<size_t, CIRC_BUFFER_SIZE> TimeStorage;

    uint16_t regBuffer;
    std::string logMessage;

  public:
    const std::string Name;
    const uint16_t Address;
    const DataUnits Unit;
    const uint16_t Divisor;

    Register(std::string cName, uint16_t cAddr, DataUnits cUnit, uint16_t cDiv, int64_t cTDelta)
        : Name{cName}, Address{cAddr}, Unit{cUnit}, Divisor{cDiv}, TimeDeltaBetweenReads{cTDelta}
    {
        ToBeRead = true;
    };
    ~Register() {}

    void MarkRegisterToBeRead() { ToBeRead = true; }

    void ReadRegister(modbus_t* conn)
    {
        if (conn != nullptr)
        {
            TimingTypes::TimeStruct now = Timing::GetTimeNow();
            if (!ToBeRead || (now.ms < (Result.LastRefresh + TimeDeltaBetweenReads)))
            {
                return;
            }
            int64_t rc = modbus_read_registers(conn, Address, 1, &regBuffer);
            if (rc == -1)
            {
                logMessage = "Can't read registers: " + std::string{modbus_strerror(errno)};
                CerboLog::AddEntry(logMessage, LogTypes::Categories::FAILURE);
            }
            else
            {
                Result.Value = (regBuffer & (1 << 15)) == 32768 ? -((regBuffer ^ 65535) + 1) : regBuffer;
                Result.LastRefresh = now.ms;
                Result.LastRefreshS = now.ms / 1000;
            }
            if (ToBeSaved)
            {
                SafeToBuffer();
            }
            return;
        }
        logMessage = "Can't read registers. No connection available.";
        CerboLog::AddEntry(logMessage, LogTypes::Categories::FAILURE);
    }

    void SafeToBuffer()
    {
        ValueStorage.AppendData(Result.Value);
        TimeStorage.AppendData(Result.LastRefresh);
    }

    const RegisterResult GetResult() const { return Result; }

    const CircularBuffer<double, CIRC_BUFFER_SIZE>& GetRTValues() const { return ValueStorage; }
    const CircularBuffer<size_t, CIRC_BUFFER_SIZE>& GetRTTimes() const { return TimeStorage; }
};

class ModbusUnit
{
  private:
    std::map<uint16_t, Register> registers;
    std::string logMessage;

  public:
    const uint16_t id;
    const std::string name;
    ModbusUnit(uint16_t cId, std::string cName) : id{cId}, name{cName} {};
    ~ModbusUnit() {}

    void AddRegister(Register registerToAdd) { registers.emplace(registerToAdd.Address, registerToAdd); }

    void ReadRegisters(modbus_t* conn)
    {
        modbus_set_slave(conn, id);
        for (auto& [regId, reg] : registers)
        {
            reg.ReadRegister(conn);
        }
    }

    const std::map<uint16_t, Register>& GetRegisters() const { return registers; }
};

class CerboModbus
{
    using State = ModbusTypes::ConnectionState;

  private:
    static inline std::string ipAdress{"192.168.178.110"};
    static inline uint16_t port{502};
    static inline modbus_t* connection = nullptr;
    static inline std::string logMessage;
    static inline bool readingActive;
    static inline bool unitsCreated;

    static inline std::map<Devices, ModbusUnit> units;
    static inline ConnectionState connectionState{ConnectionState::OFFLINE};

    CerboModbus() {}
    ~CerboModbus() {}

    static bool InitConnection()
    {
        connection = modbus_new_tcp(ipAdress.c_str(), port);
        if (connection == nullptr)
        {
            logMessage = "Creating socket failed.";
            CerboLog::AddEntry(logMessage, LogTypes::Categories::FAILURE);
            return false;
        }
        connectionState = ConnectionState::SESSION;
        AddAllUnits();
        return true;
    }

    static void AddAllUnits()
    {
        ModbusUnit System{100, "System"};
        System.AddRegister(Register{"Leistung String 1", 3724, DataUnits::WATT, 1, 2500});
        System.AddRegister(Register{"Leistung String 2", 3725, DataUnits::WATT, 1, 2500});
        System.AddRegister(Register{"Verbrauch L1", 817, DataUnits::WATT, 1, 2500});
        System.AddRegister(Register{"Verbrauch L2", 818, DataUnits::WATT, 1, 2500});
        System.AddRegister(Register{"Verbrauch L3", 819, DataUnits::WATT, 1, 2500});
        AddUnit(Devices::SYSTEM, System);

        ModbusUnit Battery{225, "Akku"};
        Battery.AddRegister(Register{"Spannung", 259, DataUnits::VOLT, 100, 2500});
        Battery.AddRegister(Register{"Strom", 261, DataUnits::AMPERE, 10, 2500});
        Battery.AddRegister(Register{"SOC", 266, DataUnits::PERCENT, 10, 20000});
        AddUnit(Devices::BATTERY, Battery);

        ModbusUnit VEBus{227, "VEBus"};
        VEBus.AddRegister(Register{"Spannung Netz L1", 3, DataUnits::VOLT, 10, 5000});
        VEBus.AddRegister(Register{"Spannung Netz L2", 4, DataUnits::VOLT, 10, 5000});
        VEBus.AddRegister(Register{"Spannung Netz L3", 5, DataUnits::VOLT, 10, 5000});
        AddUnit(Devices::VEBUS, VEBus);
        unitsCreated = true;
    }

  public:
    static void Connect()
    {
        bool temp;
        if (connection == nullptr)
        {
            temp = InitConnection();
        }
        if (temp)
        {
            if (modbus_connect(connection) == -1)
            {
                logMessage = "Connection failed: " + std::string{modbus_strerror(errno)} +
                             "\nIf no error is printed, check if target device is resposive at: " + ipAdress + ":" +
                             std::to_string(port);
                CerboLog::AddEntry(logMessage, LogTypes::Categories::FAILURE);
                modbus_free(connection);
                return;
            }
            CerboLog::AddEntry("Successfully connected.", LogTypes::Categories::SUCCESS);
            connectionState = ConnectionState::CONNECTED;
        }
    }

    static void ReadAll()
    {
        if (connectionState >= ConnectionState::CONNECTED)
        {
            for (auto& [unitEnum, unit] : units)
            {
                unit.ReadRegisters(connection);
            }
        }
    }

    static void AddUnit(Devices toAddEnum, ModbusUnit toAddUnit) { units.emplace(toAddEnum, toAddUnit); }

    static State GetConnectionState() { return connectionState; }

    static bool GetReadingActive() { return readingActive; }

    static const ModbusUnit& GetUnit(ModbusTypes::Devices _unit) { return units.at(_unit); }

    static bool GetUnitsAreCreated() { return unitsCreated; }

    static void ToggleReadingActive() { readingActive = !readingActive; }

    static void Disconnect()
    {
        modbus_close(connection);
        modbus_free(connection);
        connection = nullptr;
        CerboLog::AddEntry("Successfully disconnected.", LogTypes::Categories::SUCCESS);
        connectionState = ConnectionState::OFFLINE;
    }
};
