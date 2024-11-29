#pragma once

#include "imgui.h"

#include <d3d11.h>
#include <map>
#include <numeric>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace GUITypes
{
enum class IconVariants
{
    CONNECT,
    READ_DATA,
    DISCONNECT,
    BAR,
    LINE,
    PAUSE
};

struct Icon
{
    IconVariants IconType{};
    std::string RelPath{};
};

struct IconData
{
    ID3D11ShaderResourceView* texturePtr{};
    int width{};
    int height{};
};

const std::unordered_map<uint16_t, Icon> IconLookUp{
    {0, Icon{IconVariants::CONNECT, "/ressource/icons/connect.png"}},
    {1, Icon{IconVariants::READ_DATA, "/ressource/icons/readdata.png"}},
    {2, Icon{IconVariants::DISCONNECT, "/ressource/icons/disconnect.png"}},
    {3, Icon{IconVariants::BAR, "/ressource/icons/bar.png"}},
    {4, Icon{IconVariants::LINE, "/ressource/icons/line.png"}},
    {5, Icon{IconVariants::PAUSE, "/ressource/icons/pause.png"}}};

enum class DataSource
{
    SSH,
    API,
    MODBUS
};
} // namespace GUITypes

namespace PDTypes
{
static constexpr uint16_t noDragSourceTypes = 4;
enum class DragSource
{
    SSHRAW,
    SSHCOMPUTED,
    API,
    MODBUS,
};

const std::map<uint16_t, DragSource> DragSourceNumberMap = {
    {0, DragSource::SSHRAW}, {1, DragSource::SSHCOMPUTED}, {2, DragSource::API}, {3, DragSource::MODBUS}};

const std::map<DragSource, std::string> DragSourceMap = {{DragSource::SSHRAW, "SSHRAW"},
                                                         {DragSource::SSHRAW, "SSHCOMPUTED"},
                                                         {DragSource::API, "API"},
                                                         {DragSource::MODBUS, "MOD"}};

struct DataInPlot
{
    std::unordered_map<DragSource, bool> map;
};

struct DragDrop
{
    std::string SourceType{};
    uint16_t DataIndex{};
};

struct BarHover
{
    int16_t iHighL{-1};
    bool inBoundY{false};
    bool isHovered{false};
};

struct MaxValues
{
    int32_t xmax{std::numeric_limits<int32_t>::min()};
    int32_t xmin{std::numeric_limits<int32_t>::max()};
    float ymax{std::numeric_limits<float>::min()};
    float ymin{std::numeric_limits<float>::max()};
};

// Überall DataType statt int16_t oder whatever verwenden
enum class DataType
{
    LONG_INT,
    FLOAT,
};

enum class PlotTypes
{
    BARS,
    LINE
};

struct HelperData
{
    uint16_t TargetChunkSize;
    uint16_t Progress;
    uint16_t HeaderEnd;
    std::vector<std::string> Header;
    uint16_t DataSize;
    uint16_t RowsConverted;
    uint16_t DataCategories;
    float AvgRowSize;
    bool Completed;
    bool Error;

    HelperData(uint16_t pChunkSize)
    {
        // Use init list and init all members! (Header is missing)
        TargetChunkSize = pChunkSize;
        Progress = 0;
        HeaderEnd = 1;
        DataSize = 0;
        RowsConverted = 0;
        AvgRowSize = 0.0f;

        Completed = false;
        Error = false;
    }
};

struct BasicMetrics
{
    struct KeyPoint
    {
        int32_t Timestamp;
        float Value;
        KeyPoint(float r) : Timestamp(0), Value(r) {}
    };
    KeyPoint Max{std::numeric_limits<float>::min()};
    KeyPoint Min{std::numeric_limits<float>::max()};
    double sum{};
    float avg{};
};

struct IsInPlot
{
    bool InPlot{false};
    uint16_t SubPlotIndex{0};
    uint16_t PlotIndex{0};
};

struct Entries
{
    std::vector<float> Values;
    IsInPlot PlotInfo;
    BasicMetrics AN;
    Entries()
    {
        Values.clear(); // <- This is completely useless, Values is not even initialized here.
        AN = BasicMetrics();
    }
};

struct EnergyStruct
{
    struct DailyData
    {
        std::unordered_map<std::string, Entries> Es;
        std::vector<int32_t> Times;
        DailyData()
        {
            Es.clear();    // useless, instead initialize members
            Times.clear(); // useless, instead initialize members
        }
    };
    struct VRMData
    {
        int16_t a{0};
    };
    DailyData Daily;
    VRMData VRM;

    EnergyStruct()
    {
        // use initializer list or member inits instead
        Daily = DailyData();
        VRM = VRMData();
    }
};
} // namespace PDTypes

namespace LogTypes
{
enum class Categories
{
    SUCCESS,
    FAILURE,
    INFORMATION
};

struct Log
{
    std::string Timestamp{};
    std::string Message{};
    Categories Category{};
};
} // namespace LogTypes

namespace TimingTypes
{
struct TimeStruct
{
    std::string msString;
    int64_t ms;

    // I don't like ctor for struct with all public members. Use init lists instead
    TimeStruct(std::string cMsString, int64_t cMs) : msString{cMsString}, ms{cMs} {}
};
} // namespace TimingTypes

namespace SSHTypes
{
enum ConnectionState
{
    OFFLINE,
    SESSION,
    CONNECTED,
    AUTHENTICATED,
    CHANNEL,
    CHANNEL_SESSION,
    EXECUTED_CMD,
    READ_RESULT
};

enum class ComparisonType
{
    GR,
    GREQ,
    EQ,
    INEQ,
    SMEQ,
    SM
};

const std::unordered_map<ConnectionState, uint16_t> ProgressLookup{{ConnectionState::OFFLINE, 0},
                                                                   {ConnectionState::SESSION, 1},
                                                                   {ConnectionState::CONNECTED, 2},
                                                                   {ConnectionState::AUTHENTICATED, 3},
                                                                   {ConnectionState::CHANNEL, 4},
                                                                   {ConnectionState::CHANNEL_SESSION, 5},
                                                                   {ConnectionState::EXECUTED_CMD, 6},
                                                                   {ConnectionState::READ_RESULT, 7}};
} // namespace SSHTypes

namespace ModbusTypes
{
enum ConnectionState
{
    OFFLINE,
    SESSION,
    CONNECTED,
    AUTHENTICATED,
    EXECUTED_CMD,
    READ_RESULT
};

const std::unordered_map<ConnectionState, uint16_t> ProgressLookup{{ConnectionState::OFFLINE, 0},
                                                                   {ConnectionState::SESSION, 1},
                                                                   {ConnectionState::CONNECTED, 2},
                                                                   {ConnectionState::AUTHENTICATED, 3},
                                                                   {ConnectionState::EXECUTED_CMD, 4},
                                                                   {ConnectionState::READ_RESULT, 5}};

enum class Devices
{
    SYSTEM,
    BATTERY,
    VEBUS
};

enum class DataUnits
{
    WATT,
    KILOWATT,
    DEGREE_C,
    VOLT,
    AMPERE,
    PERCENT
};

struct RegisterResult
{
    int64_t LastRefresh;
    double Value;
};
} // namespace ModbusTypes
