#pragma once

#include "cerboLogger.h"
#include "dataHandler.h"

#include "imgui.h"
#include "implot.h"
#include "implot_internal.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <map>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace CerboPlots
{
class Visualizer
{
  private:
    PDTypes::DragDrop DropTarget;

    struct EntryInfo
    {
        // uint16 is hella small for a size
        uint16_t Size;
        uint16_t DataIndex;
        std::string Name;
        PDTypes::PlotTypes PlotType;
        PDTypes::DragSource DragSourceType;
        PDTypes::BasicMetrics Metrics;
    };

    struct PlotEntry
    {
        EntryInfo Info;
        std::vector<int32_t> SourceX;
        std::vector<float> SourceY;

        PlotEntry(EntryInfo info, std::vector<int32_t> x, std::vector<float> y)
            : Info{std::move(info)}, SourceX{std::move(x)}, SourceY{std::move(y)}
        {
        }
    };

    std::vector<std::vector<PlotEntry>> SubPlots;
    PDTypes::EnergyStruct ED;

    void
    ReceiveDragDropTarget(PDTypes::EnergyStruct& ED, const char* dropType, const std::vector<PlotEntry>& plotEntries)
    {
        // dropData muss mehr Informationen beinhalten!
        if (dropType != nullptr)
        {
            if (ImGui::BeginDragDropTarget())
            {
                if (DropTargetValid(ED, plotEntries))
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(dropType))
                    {
                        IM_ASSERT(payload->DataSize == sizeof(PDTypes::DragDrop));
                        // thats some s***
                        // I don't have imgui so I can't see the ImGuiPayload type. Can you safely cast its "Data"
                        // member to a pointer of your type DragDrop??
                        const PDTypes::DragDrop dropData = *static_cast<const PDTypes::DragDrop*>(payload->Data);

                        PDTypes::Entries& selectedEntry = ED.Daily.Es[SSHDataHandler::GetPlotKey(dropData.DataIndex)];
                        AddPlot(0,
                                ED.Daily.Times,
                                selectedEntry.Values,
                                ED.Daily.Times.size(),
                                PDTypes::PlotTypes::BARS,
                                PDTypes::DragSource::SSHRAW,
                                dropData.DataIndex,
                                selectedEntry.PlotInfo);
                        CerboLog::AddEntry("Dropped TBD into the plot.", LogTypes::Categories::SUCCESS);
                    }
                }
                ImGui::EndDragDropTarget();
            }
        }
    }

    bool DropTargetValid(PDTypes::EnergyStruct& ED, const std::vector<PlotEntry>& plotEntries)
    {
        bool returnValue = false;
        const PDTypes::BasicMetrics& droppedAN = ED.Daily.Es[SSHDataHandler::GetPlotKey(DropTarget.DataIndex)].AN;

        // you need to explain this for/switch logic to me, this can probably be written cleaner.
        for (const PlotEntry& entry : plotEntries)
        {
            switch (entry.Info.PlotType)
            {
            case PDTypes::PlotTypes::BARS:
                if (returnValue)
                {
                    return false;
                }
                if (entry.Info.Metrics.Max.Value >= droppedAN.Min.Value ||
                    entry.Info.Metrics.Min.Value >= droppedAN.Max.Value)
                {
                    returnValue = true;
                }
                break;
            case PDTypes::PlotTypes::LINE:
                returnValue = true;
                break;
            default:
                break;
            }
        }
        return true;
    }

    void PrepareSubPlot(const std::vector<PlotEntry>& subPlot)
    {
        PDTypes::MaxValues plotExtremes = GetXYExtremes(subPlot);

        ImPlot::SetupAxes(
            "Zeit", "kWh", ImPlotAxisFlags_NoGridLines, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoGridLines);
        ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Time);
        ImPlot::SetupAxisFormat(ImAxis_Y1, "%.2f");
        ImPlot::SetupAxisLimits(ImAxis_Y1, plotExtremes.ymin, plotExtremes.ymax, ImPlotCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_X1, plotExtremes.xmin, plotExtremes.xmax);
        ImPlot::SetupAxisLimitsConstraints(ImAxis_X1, plotExtremes.xmin, plotExtremes.xmax);
        ImPlot::SetupAxisZoomConstraints(ImAxis_X1, 86400, plotExtremes.xmax - plotExtremes.xmin);

        // WORKAROUND RESIZING
        ImPlot::TagY(plotExtremes.xmax, ImVec4(0.1, 0.1, 0.1, 1), true);
        ImPlot::TagY(plotExtremes.ymax, ImVec4(0.1, 0.1, 0.1, 1), true);
    }

    void PlotInSubPlot(const PlotEntry& plotEntry)
    {
        switch (plotEntry.Info.PlotType)
        {
        case PDTypes::PlotTypes::BARS:
            PlotProperBars(
                plotEntry.Info.Name, plotEntry.SourceX, plotEntry.SourceY, plotEntry.Info.Size, plotEntry.Info.Metrics);

            break;
        case PDTypes::PlotTypes::LINE:
            break;
        default:
            break;
        }
    }

    PDTypes::MaxValues GetXYExtremes(const std::vector<PlotEntry>& plots)
    {
        PDTypes::MaxValues returnStruct{};
        for (const PlotEntry& Plot : plots)
        {
            if (Plot.Info.Metrics.Min.Value < returnStruct.ymin)
            {
                returnStruct.ymin = Plot.Info.Metrics.Min.Value;
            }
            if (Plot.Info.Metrics.Max.Value > returnStruct.ymax)
            {
                returnStruct.ymax = Plot.Info.Metrics.Max.Value;
            }
            if (Plot.SourceX[0] < returnStruct.xmin)
            {
                returnStruct.xmin = Plot.SourceX[0];
            }
            // why is not the size of sourceX used but the size stored in another member of Plot?
            if (Plot.SourceX.back() > returnStruct.xmax)
            {
                returnStruct.xmax = Plot.SourceX.back();
            }
        }
        return returnStruct;
    }

    void FinishingTouches(const std::vector<PlotEntry>& subPlot)
    {
        if (subPlot.size() > 1)
        {
            ImDrawList* drawList = ImPlot::GetPlotDrawList();
            const PDTypes::MaxValues plotExtremes = GetXYExtremes(subPlot);
            const ImVec2 start = ImPlot::PlotToPixels(plotExtremes.xmin, 0);
            const ImVec2 end = ImPlot::PlotToPixels(plotExtremes.xmax, 0);
            drawList->AddLine(start, end, ImGui::GetColorU32(ImVec4(1, 1, 1, 0.3)));
        }
    }

    // Needs refactor
    void PlotProperBars(std::string plotName,
                        const std::vector<int32_t>& xdata,
                        const std::vector<float>& ydata,
                        int16_t count,
                        PDTypes::BasicMetrics AN)
    {
        ImDrawList* drawList = ImPlot::GetPlotDrawList();
        if (ImPlot::BeginItem(plotName.c_str()))
        {
            PDTypes::BarHover hoverData = FindHoveredBar(xdata, ydata, count);
            ImU32 colorBasic = ImGui::GetColorU32(ImVec4(0.113f, 0.153, 0.261f, 1.000f));
            ImU32 colorHighL = ImGui::GetColorU32(ImVec4(0.156f, 0.261f, 0.651f, 1.000f));
            ImU32 colorShade = ImGui::GetColorU32(ImVec4(0.000f, 0.000f, 0.000f, 0.300f));

            const int32_t noShadingBars = 10;
            ImU32 color = colorBasic;
            float leftX, rightX;

            assert(count >= xdata.size());
            assert(count >= ydata.size());

            for (int32_t i = 0; i < count; ++i)
            {
                hoverData.inBoundY&& hoverData.iHighL == i ? color = colorHighL : color = colorBasic;
                leftX = xdata[i];
                if (i > 0)
                {
                    leftX = xdata[i - 1];
                    xdata[i] - xdata[i - 1] < 86400 ? rightX = leftX + 86400 : rightX = xdata[i];
                }
                else
                {
                    rightX = leftX;
                    leftX -= 86400;
                }

                if ((hoverData.iHighL > -1) && (i > 0) && (hoverData.iHighL - noShadingBars + 1 < i) &&
                    (i < hoverData.iHighL + noShadingBars - 1) && ((i - 1) != hoverData.iHighL || !hoverData.inBoundY))
                {
                    ImVec2 P3 = ImPlot::PlotToPixels(leftX, 0);
                    ImVec2 P4 = ImPlot::PlotToPixels(leftX - 15000, ydata[i - 1]);
                    drawList->AddRectFilled(P3, P4, colorShade);
                }

                ImVec2 P1 = ImPlot::PlotToPixels(leftX, 0);
                ImVec2 P2 = ImPlot::PlotToPixels(rightX, ydata[i]);
                drawList->AddRectFilled(P1, P2, color);
            }
            ImPlot::EndItem();
            AddBarCosmetics(xdata, ydata, count, drawList, hoverData);
        }
    }

    PDTypes::BarHover
    FindHoveredBar(const std::vector<int32_t>& xValues, const std::vector<float>& yValues, int16_t dataCount)
    {
        PDTypes::BarHover returnData{};

        if (ImPlot::IsPlotHovered())
        {
            returnData.isHovered = true;
            ImPlotPoint mouse = ImPlot::GetPlotMousePos();
            time_t mouseXtime = ImPlotTime::FromDouble(mouse.x).S;
            int32_t mouseX = static_cast<int32_t>(mouseXtime);

            // FIND CORRESPONDING BAR
            float percentile = (float)(mouseX - xValues[0]) / (float)(xValues[dataCount - 1] - xValues[0]);
            int32_t startSearch;
            if (percentile > 0 && percentile < 1)
            {
                startSearch = static_cast<int32_t>(percentile * (dataCount - 1));
                for (; startSearch < dataCount - 1; startSearch++)
                {
                    if (mouseX > (xValues[startSearch] - 86400) && mouseX <= xValues[startSearch])
                    {
                        returnData.iHighL = startSearch;
                        if (abs(mouse.y) < abs(yValues[startSearch]))
                        {
                            returnData.inBoundY = true;
                            break;
                        }
                    }
                }
            }
        }
        return returnData;
    }

    void AddBarCosmetics(const std::vector<int32_t>& xValues,
                         const std::vector<float>& yValues,
                         int16_t dataCount,
                         ImDrawList* plotDrawList,
                         PDTypes::BarHover hoverData)
    {

        if (hoverData.isHovered)
        {
            ImPlotPoint mouse = ImPlot::GetPlotMousePos();
            time_t mouseXtime = ImPlotTime::FromDouble(mouse.x).S;
            int32_t mouseX = static_cast<int32_t>(mouseXtime);

            float tool_l = ImPlot::PlotToPixels(mouse.x - 50 * 1.5, mouse.y).x;
            float tool_r = ImPlot::PlotToPixels(mouse.x + 50 * 1.5, mouse.y).x;
            float tool_t = ImPlot::GetPlotPos().y;
            float tool_b = tool_t + ImPlot::GetPlotSize().y;
            ImPlot::PushPlotClipRect();
            plotDrawList->AddRectFilled(ImVec2(tool_l, tool_t), ImVec2(tool_r, tool_b), IM_COL32(128, 128, 128, 64));
            ImPlot::PopPlotClipRect();
        }

        char buffer[64];
        ImPlotRect limits = ImPlot::GetPlotLimits(ImAxis_X1); // X1 axis (default x-axis)
        ImPlotDateTimeSpec format = {};
        format.Date = ImPlotDateFmt_DayMo;
        format.Time = ImPlotTimeFmt_None;

        ImVec2 plotOrigin = ImPlot::GetPlotPos();
        ImVec2 plotSize = ImPlot::GetPlotSize();

        float xTag = hoverData.iHighL > 0
                         ? xValues[hoverData.iHighL] - 0.5 * (xValues[hoverData.iHighL] - xValues[hoverData.iHighL - 1])
                         : xValues[hoverData.iHighL] - 0.5 * 86400;
        ImVec2 x = ImPlot::PlotToPixels(limits.X.Min, yValues[hoverData.iHighL]);
        ImVec2 y = ImPlot::PlotToPixels(limits.X.Max, yValues[hoverData.iHighL]);

        /* if (hoverData.inBoundY) {
            plotDrawList->AddLine(x, y, ImGui::GetColorU32(ImVec4(1, 1, 1, 0.3)));
            x = ImPlot::PlotToPixels(xTag, yValues[hoverData.iHighL]);
            y = ImPlot::PlotToPixels(xTag, limits.Y.Min);
            plotDrawList->AddLine(x, y, ImGui::GetColorU32(ImVec4(1, 1, 1, 0.3)));
        } */

        if (hoverData.iHighL > -1)
        {
            xTag = xValues[hoverData.iHighL] - 0.5 * (xValues[hoverData.iHighL] - xValues[hoverData.iHighL - 1]);
            ImPlot::TagY(yValues[hoverData.iHighL], ImVec4(1, 0, 0, 1), true);
        }
        else
        {
            xTag = ImPlot::PixelsToPlot(ImVec2(plotOrigin.x + plotSize.x, plotOrigin.y + plotSize.y)).x - 100;
        }
        if (plotOrigin.x > ImPlot::PlotToPixels(ImPlotPoint(ImVec2(xTag, 0))).x)
        {
            xTag = ImPlot::PixelsToPlot(plotOrigin).x + 100;
        }

        ImPlot::FormatDateTime(xTag, buffer, sizeof(buffer), format);
        ImPlot::TagX(xTag, ImVec4(1, 0, 0, 1), buffer);
    }

    // Can this function be used with vectors aswell?
    template <size_t Sx, size_t Sy>
    void PlotProperLineGraph(const std::string& plotName,
                             const std::array<size_t, Sx>& xValues,
                             const std::array<double, Sy>& yValues) const
    {
        ImDrawList* drawList = ImPlot::GetPlotDrawList();
        const int32_t size = std::min(Sx, Sy);
        ImU32 color = ImGui::GetColorU32(ImVec4{1, 1, 1, 1});
        if (size == 0)
        {
            return;
        }
        if (ImPlot::BeginItem(plotName.c_str()))
        {
            for (int32_t i = 1; i < size; i++)
            {
                drawList->AddLine(ImVec2{xValues[i - 1], yValues[i - 1]}, ImVec2{xValues[i], yValues[i]}, color);
                drawList->AddRectFilled(ImVec2{xValues[i - 1], 500}, ImVec2{xValues[i], 400}, color);
            }
        }
    }

  public:
    void GetData(const std::string& rawData)
    {
        if (rawData == "")
        {
            return;
        }
        if (SSHDataHandler::ConversionPending() || SSHDataHandler::ReadyForConversion())
        {
            SSHDataHandler::FormatData(ED, rawData);
            if (SSHDataHandler::DataAvailable())
            {
                const std::vector<int32_t>& xdata = ED.Daily.Times;
                const std::vector<float>& ydata = ED.Daily.Es[SSHDataHandler::GetPlotKey(1)].Values;
                PDTypes::IsInPlot& inPlotInfo = ED.Daily.Es[SSHDataHandler::GetPlotKey(1)].PlotInfo;
                const int16_t& size = ED.Daily.Times.size();
                SSHDataHandler::ComputeAnalytics(ED);
                AddPlot(0, xdata, ydata, size, PDTypes::PlotTypes::BARS, PDTypes::DragSource::SSHRAW, 1, inPlotInfo);
            }
        }
    }

    void PlotGraph()
    {
        ImVec2 plotSize = ImGui::GetContentRegionAvail();
        if (SSHDataHandler::ConversionPending() || !SSHDataHandler::DataAvailable())
        {
            return;
        }
        ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(20.0f, 20.0f)); // 20px padding

        if (ImPlot::BeginPlot("Energiedaten", plotSize))
        {
            for (std::vector<PlotEntry> SubPlot : SubPlots)
            {
                ReceiveDragDropTarget(ED, DropTarget.SourceType.c_str(), SubPlot);
                PrepareSubPlot(SubPlot);

                for (PlotEntry Plot : SubPlot)
                {
                    PlotInSubPlot(Plot);
                }
                FinishingTouches(SubPlot);
            }
            ImPlot::EndPlot();
        }
    }

    void PlotRealTimeData() const
    {
        if (!CerboModbus::GetUnitsAreCreated())
        {
            return;
        }
        ImVec2 plotSize = ImGui::GetContentRegionAvail();
        if (ImPlot::BeginPlot("Echtzeitdaten", plotSize))
        {
            PrepareUnitDataPlot();
            PlotUnitData(CerboModbus::GetUnit(ModbusTypes::Devices::SYSTEM));
            PlotUnitData(CerboModbus::GetUnit(ModbusTypes::Devices::BATTERY));
            PlotUnitData(CerboModbus::GetUnit(ModbusTypes::Devices::VEBUS));
            ImPlot::EndPlot();
        }
    }

    void PlotUnitData(const ModbusUnit& targetUnit) const
    {

        const std::map<uint16_t, Register>& targetRegisters = targetUnit.GetRegisters();

        for (const auto& [lId, lRegister] : targetRegisters)
        {
            const CircularBuffer<size_t, CIRC_BUFFER_SIZE>& timesBuffer = lRegister.GetRTTimes();
            const CircularBuffer<double, CIRC_BUFFER_SIZE>& valuesBuffer = lRegister.GetRTValues();
            std::array<size_t, CIRC_BUFFER_SIZE> times = timesBuffer.GetData();
            std::array<double, CIRC_BUFFER_SIZE> values = valuesBuffer.GetData();
            PlotProperLineGraph(lRegister.Name, times, values);
        }
    }

    void PrepareUnitDataPlot() const
    {
        PDTypes::MaxValues plotExtremes;
        plotExtremes.xmax = Timing::GetTimeNow().ms / 1000;
        plotExtremes.xmin = plotExtremes.xmax - 1000;
        plotExtremes.ymax = 1000;
        plotExtremes.ymin = -500;

        ImPlot::SetupAxes(
            "Zeit", "kWh", ImPlotAxisFlags_NoGridLines, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoGridLines);
        ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Time);
        ImPlot::SetupAxisFormat(ImAxis_Y1, "%.2f");
        ImPlot::SetupAxisLimits(ImAxis_Y1, plotExtremes.ymin, plotExtremes.ymax, ImPlotCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_X1, plotExtremes.xmin, plotExtremes.xmax);
        ImPlot::SetupAxisLimitsConstraints(ImAxis_X1, plotExtremes.xmin, plotExtremes.xmax);
        ;
    }

    void PrepareDropTarget(const PDTypes::DragDrop& DragSource) { DropTarget = DragSource; }

    void AddPlot(const int16_t subPlotIndex,
                 const std::vector<int32_t>& xdata,
                 const std::vector<float>& ydata,
                 const uint16_t& size,
                 PDTypes::PlotTypes type,
                 PDTypes::DragSource dragSourceType,
                 uint16_t dataIndex,
                 PDTypes::IsInPlot& inPlotInfo)
    {
        const std::string pKey = SSHDataHandler::GetPlotKey(dataIndex);
        EntryInfo toAddInfo = EntryInfo{
            size, dataIndex, SSHDataHandler::GetPlotName(dataIndex), type, dragSourceType, ED.Daily.Es[pKey].AN};
        PlotEntry toAddPlot = PlotEntry{std::move(toAddInfo), xdata, ydata};
        if (SubPlots.size() == 0)
        {
            SubPlots.push_back(std::vector<PlotEntry>());
        }
        if (SubPlots.size() > subPlotIndex)
        {
            SubPlots[subPlotIndex].push_back(std::move(toAddPlot));
            inPlotInfo.SubPlotIndex = subPlotIndex;
        }
        else
        {
            assert(false && "Fix me");
            SubPlots.resize(subPlotIndex + 1);
            SubPlots[subPlotIndex].push_back(std::move(toAddPlot));
            inPlotInfo.SubPlotIndex = subPlotIndex;
        }
        inPlotInfo.InPlot = true;
    }

    void RemovePlot(uint16_t subPlotIndex, uint16_t plotIndex, uint16_t dataIndex)
    {
        std::vector<PlotEntry>& entryRef = SubPlots.at(subPlotIndex);
        entryRef.erase(entryRef.begin() + plotIndex);
        ED.Daily.Es[SSHDataHandler::GetPlotKey(dataIndex)].PlotInfo.InPlot = false;
    }

    bool CheckPlotInSubplot(const uint16_t subPlotIndex, const uint16_t dataIndex, PDTypes::DragSource dataType) const
    {
        PDTypes::IsInPlot toCheck;
        switch (dataType)
        {
        case PDTypes::DragSource::SSHRAW:
            toCheck = ED.Daily.Es.at(SSHDataHandler::GetPlotKey(dataIndex)).PlotInfo;
            if (toCheck.SubPlotIndex == subPlotIndex && toCheck.InPlot)
            {
                return true;
            }
            break;
        case PDTypes::DragSource::SSHCOMPUTED:
            return false;
        case PDTypes::DragSource::MODBUS:
            return false;
        case PDTypes::DragSource::API:
            return false;
        default:
            return true;
            break;
        }
        return false;
    }

    // Use const member fn'S to explicity state, that it will not alter your instances data members!
    // Could probably be applied to more functions but I don't wanna check every function, given your naming scheme
    // doesn't even enable to quickly identify class members.
    PDTypes::IsInPlot GetIsInPlotInfo(uint16_t dataIndex) const
    {
        return ED.Daily.Es.at(SSHDataHandler::GetPlotKey(dataIndex)).PlotInfo;
    }

    uint16_t GetSubPlotCount() const { return SubPlots.size(); }

    void ResetData()
    {
        DropTarget = PDTypes::DragDrop{"", 0};
        ED = PDTypes::EnergyStruct();
    }
};
} // namespace CerboPlots