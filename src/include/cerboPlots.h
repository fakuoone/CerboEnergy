#pragma once

#include "dataHandler.h"
#include "cerboLogger.h"

#include "implot.h"
#include "implot_internal.h"
#include "imgui.h"

#include <string>
#include <sstream>
#include <unordered_map>
#include <map>
#include <stdexcept>
#include <numeric>
#include <cmath>
#include <chrono>

namespace CerboPlots {
    class Visualizer {
    private:
        PDTypes::DragDrop DropTarget;

        struct EntryInfo {
            uint16_t Size;
            uint16_t DataIndex;
            std::string Name;
            PDTypes::PlotTypes PlotType;
            PDTypes::DragSource DragSourceType;
            PDTypes::BasicMetrics Metrics;
        };

        struct PlotEntry {
            EntryInfo Info;
            std::vector<int32_t> SourceX;
            std::vector<float> SourceY;

            PlotEntry(EntryInfo info, std::vector<int32_t> x, std::vector<float> y)
                : Info(info), SourceX(x), SourceY(y) {}
        };

        std::vector<std::vector<PlotEntry>> SubPlots;
        PDTypes::EnergyStruct ED;

        void ReceiveDragDropTarget(PDTypes::EnergyStruct& ED, const char* dropType, std::vector<PlotEntry> plotEntries) {
            // dropData muss mehr Informationen beinhalten!
            if (dropType != nullptr) {
                if (ImGui::BeginDragDropTarget()) {
                    if (DropTargetValid(ED, plotEntries)) {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(dropType)) {
                            IM_ASSERT(payload->DataSize == sizeof(PDTypes::DragDrop));
                            PDTypes::DragDrop dropData = *(const PDTypes::DragDrop*)payload->Data;
                            dropType = nullptr;
                            int16_t i = dropData.DataIndex;
                            PDTypes::Entries& selectedEntry = ED.Daily.Es[SSHDataHandler::GetPlotKey(i)];
                            AddPlot(0, ED.Daily.Times, selectedEntry.Values, ED.Daily.Times.size(), PDTypes::PlotTypes::BARS, PDTypes::DragSource::SSHRAW, i, selectedEntry.PlotInfo);
                            std::string logText = "Dropped TBD into the plot.";
                            CerboLog::AddEntry(logText, LogTypes::Categories::SUCCESS);
                        }
                    }
                    ImGui::EndDragDropTarget();
                }
            }
        }

        bool DropTargetValid(PDTypes::EnergyStruct& ED, std::vector<PlotEntry> plotEntries) {
            bool returnValue = false;
            const PDTypes::BasicMetrics droppedAN = ED.Daily.Es[SSHDataHandler::GetPlotKey(DropTarget.DataIndex)].AN;
            for (PlotEntry entry : plotEntries) {
                switch (entry.Info.PlotType) {
                case PDTypes::PlotTypes::BARS:
                    if (returnValue) {
                        return false;
                    }
                    if (entry.Info.Metrics.Max.Value >= droppedAN.Min.Value || entry.Info.Metrics.Min.Value >= droppedAN.Max.Value) {
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

        bool PrepareSubPlot(std::vector<PlotEntry> subPlot) {
            if (subPlot.size() < 0) {
                return false;
            }
            PDTypes::MaxValues plotExtremes = GetXYExtremes(subPlot);

            ImPlot::SetupAxes("Zeit", "kWh", ImPlotAxisFlags_NoGridLines, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoGridLines);
            ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Time);
            ImPlot::SetupAxisFormat(ImAxis_Y1, "%.2f");
            ImPlot::SetupAxisLimits(ImAxis_Y1, plotExtremes.ymin, plotExtremes.ymax, ImPlotCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_X1, plotExtremes.xmin, plotExtremes.xmax);
            ImPlot::SetupAxisLimitsConstraints(ImAxis_X1, plotExtremes.xmin, plotExtremes.xmax);
            ImPlot::SetupAxisZoomConstraints(ImAxis_X1, 86400, plotExtremes.xmax - plotExtremes.xmin);

            // WORKAROUND RESIZING
            ImPlot::TagY(plotExtremes.xmax, ImVec4(0.1, 0.1, 0.1, 1), true);
            ImPlot::TagY(plotExtremes.ymax, ImVec4(0.1, 0.1, 0.1, 1), true);

            return true;
        }

        void PlotInSubPlot(PlotEntry plotEntry) {
            switch (plotEntry.Info.PlotType) {
            case PDTypes::PlotTypes::BARS:
                PlotProperBars(plotEntry.Info.Name, plotEntry.SourceX, plotEntry.SourceY, plotEntry.Info.Size, plotEntry.Info.Metrics);
                break;
            case PDTypes::PlotTypes::LINE:
                break;
            default:
                break;
            }
        }

        PDTypes::MaxValues GetXYExtremes(std::vector<PlotEntry> plots) {
            PDTypes::MaxValues returnStruct{};
            for (PlotEntry Plot : plots) {
                if (Plot.Info.Metrics.Min.Value < returnStruct.ymin) {
                    returnStruct.ymin = Plot.Info.Metrics.Min.Value;
                }
                if (Plot.Info.Metrics.Max.Value > returnStruct.ymax) {
                    returnStruct.ymax = Plot.Info.Metrics.Max.Value;
                }
                if (Plot.SourceX[0] < returnStruct.xmin) {
                    returnStruct.xmin = Plot.SourceX[0];
                }
                if (Plot.SourceX[Plot.Info.Size - 1] > returnStruct.xmax) {
                    returnStruct.xmax = Plot.SourceX[Plot.Info.Size - 1];
                }
            }
            return returnStruct;
        }

        void FinishingTouches(std::vector<PlotEntry> subPlot) {
            if (subPlot.size() > 1) {
                ImDrawList* drawList = ImPlot::GetPlotDrawList();
                PDTypes::MaxValues plotExtremes = GetXYExtremes(subPlot);
                ImVec2 start = ImPlot::PlotToPixels(plotExtremes.xmin, 0);
                ImVec2 end = ImPlot::PlotToPixels(plotExtremes.xmax, 0);
                drawList->AddLine(start, end, ImGui::GetColorU32(ImVec4(1, 1, 1, 0.3)));
            }
        }

        void PlotProperBars(std::string plotName, const std::vector<int32_t>& xdata, const std::vector<float>& ydata, int16_t count, PDTypes::BasicMetrics AN) {
            ImDrawList* drawList = ImPlot::GetPlotDrawList();
            if (ImPlot::BeginItem(plotName.c_str())) {
                PDTypes::BarHover hoverData = FindHoveredBar(xdata, ydata, count);
                ImU32 colorBasic = ImGui::GetColorU32(ImVec4(0.113f, 0.153, 0.261f, 1.000f));
                ImU32 colorHighL = ImGui::GetColorU32(ImVec4(0.156f, 0.261f, 0.651f, 1.000f));
                ImU32 colorShade = ImGui::GetColorU32(ImVec4(0.000f, 0.000f, 0.000f, 0.300f));

                const int16_t noShadingBars = 10;
                ImU32 color = colorBasic;
                float leftX, rightX;

                for (int16_t i = 0; i < count - 1; ++i) {
                    hoverData.inBoundY&& hoverData.iHighL == i ? color = colorHighL : color = colorBasic;
                    leftX = xdata[i];
                    if (i > 0) {
                        leftX = xdata[i - 1];
                        xdata[i] - xdata[i - 1] < 86400 ? rightX = leftX + 86400 : rightX = xdata[i];
                    } else {
                        rightX = leftX;
                        leftX -= 86400;
                    }

                    if ((hoverData.iHighL > -1) && (i > 0) && (hoverData.iHighL - noShadingBars + 1 < i) && (i < hoverData.iHighL + noShadingBars - 1) && ((i - 1) != hoverData.iHighL || !hoverData.inBoundY)) {
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

        PDTypes::BarHover FindHoveredBar(const std::vector<int32_t>& xValues, const std::vector<float>& yValues, int16_t dataCount) {
            PDTypes::BarHover returnData{};

            if (ImPlot::IsPlotHovered()) {
                returnData.isHovered = true;
                ImPlotPoint mouse = ImPlot::GetPlotMousePos();
                time_t mouseXtime = ImPlotTime::FromDouble(mouse.x).S;
                int32_t mouseX = static_cast<int32_t>(mouseXtime);

                // FIND CORRESPONDING BAR
                float percentile = (float)(mouseX - xValues[0]) / (float)(xValues[dataCount - 1] - xValues[0]);
                int32_t startSearch;
                if (percentile > 0 && percentile < 1) {
                    startSearch = static_cast<int32_t>(percentile * (dataCount - 1));
                    for (; startSearch < dataCount - 1; startSearch++) {
                        if (mouseX > (xValues[startSearch] - 86400) && mouseX <= xValues[startSearch]) {
                            returnData.iHighL = startSearch;
                            if (abs(mouse.y) < abs(yValues[startSearch])) {
                                returnData.inBoundY = true;
                                break;
                            }
                        }
                    }
                }
            }
            return returnData;
        }

        void AddBarCosmetics(const std::vector<int32_t>& xValues, const std::vector<float>& yValues, int16_t dataCount, ImDrawList* plotDrawList, PDTypes::BarHover hoverData) {

            if (hoverData.isHovered) {
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

            float xTag = hoverData.iHighL > 0 ? xValues[hoverData.iHighL] - 0.5 * (xValues[hoverData.iHighL] - xValues[hoverData.iHighL - 1]) : xValues[hoverData.iHighL] - 0.5 * 86400;
            ImVec2 x = ImPlot::PlotToPixels(limits.X.Min, yValues[hoverData.iHighL]);
            ImVec2 y = ImPlot::PlotToPixels(limits.X.Max, yValues[hoverData.iHighL]);

            /* if (hoverData.inBoundY) {
                plotDrawList->AddLine(x, y, ImGui::GetColorU32(ImVec4(1, 1, 1, 0.3)));
                x = ImPlot::PlotToPixels(xTag, yValues[hoverData.iHighL]);
                y = ImPlot::PlotToPixels(xTag, limits.Y.Min);
                plotDrawList->AddLine(x, y, ImGui::GetColorU32(ImVec4(1, 1, 1, 0.3)));
            } */

            if (hoverData.iHighL > -1) {
                xTag = xValues[hoverData.iHighL] - 0.5 * (xValues[hoverData.iHighL] - xValues[hoverData.iHighL - 1]);
                ImPlot::TagY(yValues[hoverData.iHighL], ImVec4(1, 0, 0, 1), true);
            } else {
                xTag = ImPlot::PixelsToPlot(ImVec2(plotOrigin.x + plotSize.x, plotOrigin.y + plotSize.y)).x - 100;
            }
            if (plotOrigin.x > ImPlot::PlotToPixels(ImPlotPoint(ImVec2(xTag, 0))).x) {
                xTag = ImPlot::PixelsToPlot(plotOrigin).x + 100;
            }

            ImPlot::FormatDateTime(xTag, buffer, sizeof(buffer), format);
            ImPlot::TagX(xTag, ImVec4(1, 0, 0, 1), buffer);
        }


    public:
        Visualizer() {}

        bool GetData(std::string rawData) {
            bool completed = SSHDataHandler::FormatData(ED, rawData);
            if (completed) {
                std::vector<int32_t> xdata = ED.Daily.Times;
                std::vector<float> ydata = ED.Daily.Es[SSHDataHandler::GetPlotKey(1)].Values;
                PDTypes::IsInPlot& inPlotInfo = ED.Daily.Es[SSHDataHandler::GetPlotKey(1)].PlotInfo;
                const int16_t& size = ED.Daily.Times.size();
                SSHDataHandler::ComputeAnalytics(ED);
                AddPlot(0, xdata, ydata, size, PDTypes::PlotTypes::BARS, PDTypes::DragSource::SSHRAW, 1, inPlotInfo);
            }
            return completed;
        }

        void PlotGraph() {
            ImVec2 plotSize = ImGui::GetContentRegionAvail();
            if (SSHDataHandler::ConversionPending() || !SSHDataHandler::DataAvailable()) {
                return;
            }
            ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(20.0f, 20.0f)); // 20px padding

            if (ImPlot::BeginPlot("Energiedaten", plotSize)) {
                for (std::vector<PlotEntry> SubPlot : SubPlots) {
                    ReceiveDragDropTarget(ED, DropTarget.SourceType.c_str(), SubPlot);
                    if (PrepareSubPlot(SubPlot)) {
                        for (PlotEntry Plot : SubPlot) {
                            PlotInSubPlot(Plot);
                        }
                        FinishingTouches(SubPlot);
                    }
                }
                ImPlot::EndPlot();
            }
        }

        void PrepareDropTarget(PDTypes::DragDrop DragSource) {
            DropTarget = DragSource;
        }

        void AddPlot(const int16_t subPlotIndex, std::vector<int32_t>& xdata, std::vector<float>& ydata, const uint16_t& size, PDTypes::PlotTypes type, PDTypes::DragSource dragSourceType, uint16_t dataIndex, PDTypes::IsInPlot& inPlotInfo) {
            const std::string pName = SSHDataHandler::GetPlotName(dataIndex);
            const std::string pKey = SSHDataHandler::GetPlotKey(dataIndex);
            EntryInfo toAddInfo = EntryInfo{ size, dataIndex, pName, type, dragSourceType, ED.Daily.Es[pKey].AN };
            PlotEntry toAddPlot = PlotEntry{ toAddInfo, xdata, ydata };
            if (SubPlots.size() == 0) {
                SubPlots.push_back(std::vector<PlotEntry>());
            }
            if (SubPlots.size() > subPlotIndex) {
                SubPlots[subPlotIndex].push_back(toAddPlot);
                inPlotInfo.SubPlotIndex = subPlotIndex;
            } else {
                uint16_t subPlotSize = SubPlots.size();
                SubPlots[subPlotSize].push_back(toAddPlot);
                inPlotInfo.SubPlotIndex = subPlotSize;
            }
            inPlotInfo.InPlot = true;
        }

        void RemovePlot(uint16_t subPlotIndex, uint16_t plotIndex, uint16_t dataIndex) {
            std::vector<PlotEntry>& entryRef = SubPlots.at(subPlotIndex);
            entryRef.erase(entryRef.begin() + plotIndex);
            ED.Daily.Es[SSHDataHandler::GetPlotKey(dataIndex)].PlotInfo.InPlot = false;
        }

        bool CheckPlotInSubplot(const uint16_t subPlotIndex, const uint16_t dataIndex, PDTypes::DragSource dataType) {
            PDTypes::IsInPlot toCheck;
            switch (dataType) {
            case PDTypes::DragSource::SSHRAW:
                toCheck = ED.Daily.Es[SSHDataHandler::GetPlotKey(dataIndex)].PlotInfo;
                if (toCheck.SubPlotIndex == subPlotIndex && toCheck.InPlot) {
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

        PDTypes::IsInPlot GetIsInPlotInfo(uint16_t dataIndex) {
            return ED.Daily.Es[SSHDataHandler::GetPlotKey(dataIndex)].PlotInfo;
        }

        uint16_t GetSubPlotCount() {
            return SubPlots.size();
        }

        void ResetData() {
            DropTarget = PDTypes::DragDrop{ "", 0 };
            ED = PDTypes::EnergyStruct();
        }
    };
}