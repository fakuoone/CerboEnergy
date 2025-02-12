#pragma once

#include "imgui.h"

#include <nlohmann/json.hpp>

#include <d3d11.h>
#include <map>
#include <numeric>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#define CIRC_BUFFER_SIZE 5000

namespace LogTypes {
enum class Categories { SUCCESS, FAILURE, INFORMATION };

struct Log {
    std::string Timestamp{};
    std::string Message{};
    Categories Category{};
};
}  // namespace LogTypes

namespace TimingTypes {
typedef int64_t milliseconds;
struct TimeStruct {
    std::string msString;
    milliseconds ms;

    // I don't like ctor for struct with all public members. Use init lists instead
    TimeStruct(std::string cMsString, int64_t cMs) : msString{cMsString}, ms{cMs} {}
};
}  // namespace TimingTypes

namespace GUITypes {
enum class IconVariants { CONNECT, READ_DATA, DISCONNECT, BAR, LINE, PAUSE };

struct Icon {
    IconVariants IconType{};
    std::string RelPath{};
};

struct IconData {
    ID3D11ShaderResourceView* texturePtr{};
    int width{};
    int height{};
};

const std::unordered_map<uint16_t, Icon> IconLookUp{
    {0, Icon{IconVariants::CONNECT, "/ressource/icons/connect.png"}}, {1, Icon{IconVariants::READ_DATA, "/ressource/icons/readdata.png"}}, {2, Icon{IconVariants::DISCONNECT, "/ressource/icons/disconnect.png"}},
    {3, Icon{IconVariants::BAR, "/ressource/icons/bar.png"}},         {4, Icon{IconVariants::LINE, "/ressource/icons/line.png"}},          {5, Icon{IconVariants::PAUSE, "/ressource/icons/pause.png"}}};

enum class DataSource { SSH, API, MODBUS };
}  // namespace GUITypes

namespace PDTypes {
static constexpr uint16_t noDragSourceTypes = 4;
enum class DragSource {
    SSHRAW,
    SSHCOMPUTED,
    API,
    MODBUS,
};

const std::map<uint16_t, DragSource> DragSourceNumberMap = {{0, DragSource::SSHRAW}, {1, DragSource::SSHCOMPUTED}, {2, DragSource::API}, {3, DragSource::MODBUS}};

const std::map<DragSource, std::string> DragSourceMap = {{DragSource::SSHRAW, "SSHRAW"}, {DragSource::SSHRAW, "SSHCOMPUTED"}, {DragSource::API, "API"}, {DragSource::MODBUS, "MOD"}};

struct DataInPlot {
    std::unordered_map<DragSource, bool> map;
};

struct DragDrop {
    std::string SourceType{};
    uint16_t DataIndex{};
};

struct BarHover {
    int16_t iHighL{-1};
    bool inBoundY{false};
    bool isHovered{false};
};

struct MaxValues {
    int32_t xmax{std::numeric_limits<int32_t>::min()};
    int32_t xmin{std::numeric_limits<int32_t>::max()};
    float ymax{std::numeric_limits<float>::min()};
    float ymin{std::numeric_limits<float>::max()};
};

// Überall DataType statt int16_t oder whatever verwenden
enum class DataType {
    LONG_INT,
    FLOAT,
};

enum class PlotTypes { BARS, LINE };

struct HelperData {
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

    HelperData(uint16_t pChunkSize) {
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

struct BasicMetrics {
    struct KeyPoint {
        int32_t Timestamp;
        float Value;
        KeyPoint(float r) : Timestamp(0), Value(r) {}
    };
    KeyPoint Max{std::numeric_limits<float>::min()};
    KeyPoint Min{std::numeric_limits<float>::max()};
    double sum{};
    float avg{};
};

struct IsInPlot {
    bool InPlot{false};
    uint16_t SubPlotIndex{0};
    uint16_t PlotIndex{0};
};

struct Entries {
    struct TimeSpanSummary {
        TimingTypes::milliseconds start;
        TimingTypes::milliseconds end;
        double avg;
        double max;
        double min;
        double sum;
    };
    std::vector<float> Values;
    IsInPlot PlotInfo;
    BasicMetrics AN;
    TimeSpanSummary Summary;
    Entries() : AN{} {}
};

struct EnergyStruct {
    struct DailyData {
        std::unordered_map<std::string, Entries> Es;
        std::vector<int32_t> Times;
    };
    struct VRMData {
        int16_t a{0};
    };
    DailyData Daily;
    VRMData VRM;
};
}  // namespace PDTypes

namespace SSHTypes {
enum ConnectionState { OFFLINE, SESSION, CONNECTED, AUTHENTICATED, CHANNEL, CHANNEL_SESSION, EXECUTED_CMD, READ_RESULT };

enum class ComparisonType { GR, GREQ, EQ, INEQ, SMEQ, SM };

const std::unordered_map<ConnectionState, uint16_t> ProgressLookup{{ConnectionState::OFFLINE, 0}, {ConnectionState::SESSION, 1},         {ConnectionState::CONNECTED, 2},    {ConnectionState::AUTHENTICATED, 3},
                                                                   {ConnectionState::CHANNEL, 4}, {ConnectionState::CHANNEL_SESSION, 5}, {ConnectionState::EXECUTED_CMD, 6}, {ConnectionState::READ_RESULT, 7}};
}  // namespace SSHTypes

namespace ModbusTypes {
enum ConnectionState { OFFLINE, SESSION, CONNECTED, AUTHENTICATED, EXECUTED_CMD, READ_RESULT };

const std::unordered_map<ConnectionState, uint16_t> ProgressLookup{{ConnectionState::OFFLINE, 0},       {ConnectionState::SESSION, 1},      {ConnectionState::CONNECTED, 2},
                                                                   {ConnectionState::AUTHENTICATED, 3}, {ConnectionState::EXECUTED_CMD, 4}, {ConnectionState::READ_RESULT, 5}};

enum class Devices { SYSTEM, BATTERY, VEBUS };

enum class DataUnits { WATT, KILOWATT, DEGREE_C, VOLT, AMPERE, PERCENT };

const std::unordered_map<DataUnits, std::string> DataUnitLookup{{DataUnits::WATT, "W"}, {DataUnits::KILOWATT, "kW"}, {DataUnits::DEGREE_C, "°C"}, {DataUnits::VOLT, "V"}, {DataUnits::AMPERE, "A"}, {DataUnits::PERCENT, "%%"}};

struct RegisterResult {
    int64_t LastRefresh{0};  // Without this, things break because of uninitialized value??
    int32_t LastRefreshS{0};
    double Value{0};
};

template <typename T, size_t Size>
class CircularBuffer {
   private:
    std::array<T, Size> Data;
    size_t Head{0};
    size_t Tail{0};
    bool Full{false};
    bool Started{false};
    struct Iterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = int;
        using pointer = int*;    // or also value_type*
        using reference = int&;  // or also value_type&
        Iterator(pointer ptr) : m_ptr{ptr} {}
        reference operator*() const { return *m_ptr; }
        pointer operator->() { return m_ptr; }
        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }
        friend bool operator==(const Iterator& a, const Iterator& b) { return a.m_ptr == b.m_ptr; };
        friend bool operator!=(const Iterator& a, const Iterator& b) { return a.m_ptr != b.m_ptr; };

       private:
        pointer m_ptr;
    };

    Iterator begin() { return Iterator{&Data[Tail]}; }
    Iterator end() { return Iterator{&Data[Head]}; }

   public:
    void AppendData(const T& _data) {
        if (Started) {
            Head = (Head + 1) % Size;
            Tail = Full ? (Head + 1) % Size : 0;
            size_t diff = Head > Tail ? Head - Tail : Tail - Head;
            if (diff == Size - 1) {
                Full = true;
            }
        }
        Data[Head] = _data;
        Started = true;
    }

    const std::array<T, Size>& GetData() const { return Data; }

    const size_t GetSize() { return Data.max_size(); }

    const size_t GetHead() const { return Head; }

    const size_t GetTail() const { return Tail; }
};
}  // namespace ModbusTypes

namespace ApiTypes {
struct LoginData {
    std::string userName;
    std::string password;
    std::string token;
    int64_t siteID;
};

struct ResponseInfo {
    bool success;
    std::string errors;
    uint16_t errorCode;
};

enum ConnectionState { OFFLINE, AUTHENTICATING, AUTHENTICATED, REQUESTING, DATA_RECEIVED };

struct ConnectionData {
    LoginData login;
    ResponseInfo responseInfo;
    std::string responseString;
    nlohmann::json responseJson;
    ConnectionState state;
};

}  // namespace ApiTypes