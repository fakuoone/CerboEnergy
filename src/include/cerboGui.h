#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "cerboLogger.h"
#include "cerboModbus.h"
#include "cerboPlots.h"
#include "cerboSSH.h"
#include "dataHandler.h"
#include "dataTypes.h"
#include "vrmApi.h"

#include "imgui.h"

#include <array>
#include <d3d11.h>
#include <iomanip>
#include <iostream>
#include <sstream>

#define ICON_SIZE 32;

namespace CerboGui
{
bool LoadTextureFromMemory(ID3D11Device* deviceFromMain,
                           const void* data,
                           size_t data_size,
                           ID3D11ShaderResourceView** out_srv,
                           int* out_width,
                           int* out_height);
bool LoadTextureFromFile(ID3D11Device* renderDeviceFromMain,
                         const char* file_name,
                         ID3D11ShaderResourceView** out_srv,
                         int* out_width,
                         int* out_height);

class App
{
  private:
    uint16_t windowCount;
    ImGuiID dockSpace;
    std::unique_ptr<CerboPlots::Visualizer> VisualizerInstance;
    std::map<GUITypes::IconVariants, GUITypes::IconData> IconMap;
    ID3D11Device* renderDeviceFromMain = nullptr;

    byte IconSize = 50;

    void CreateDockSpace()
    {
        dockSpace = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
        ImGui::SetNextWindowDockID(dockSpace);
    }

    bool AddAppControlButton(const std::string& name, bool enable, GUITypes::IconData icon, bool isLastInRow)
    {
        if (AddButton(name, enable, icon, 1.0f))
        {
            return true;
        }
        if (!isLastInRow)
        {
            ImGui::SameLine();
        }
        return false;
    }

    bool AddButton(const std::string& name, bool enable, GUITypes::IconData icon, float sizeScaling)
    {
        ImGui::BeginDisabled(!enable);
        if (ImGui::ImageButton(name.c_str(),
                               (ImTextureID)(intptr_t)icon.texturePtr,
                               ImVec2(IconSize * sizeScaling, IconSize * sizeScaling)))
        {
            ImGui::EndDisabled();
            return true;
        }
        ImGui::EndDisabled();
        return false;
    }

    void CreateAppControls()
    {
        GUITypes::IconData connectIcon = IconMap.at(GUITypes::IconVariants::CONNECT);
        GUITypes::IconData disConnectIcon = IconMap.at(GUITypes::IconVariants::DISCONNECT);
        GUITypes::IconData readIcon = IconMap.at(GUITypes::IconVariants::READ_DATA);
        GUITypes::IconData pauseIcon = IconMap.at(GUITypes::IconVariants::PAUSE);
        if (ImGui::CollapsingHeader("SSH-Daten", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (AddAppControlButton("Verbindung aufbauen",
                                    CerboSSH::GetConnectionState() < SSHTypes::ConnectionState::CONNECTED,
                                    connectIcon,
                                    false))
            {
                CerboSSH::ConnectCerbo();
            }
            if (AddAppControlButton("Daten lesen",
                                    CerboSSH::GetConnectionState() >= SSHTypes::ConnectionState::CONNECTED,
                                    readIcon,
                                    false))
            {
                SSHDataHandler::ResetSSHData();
                CerboSSH::ReadEnergyFile();
                VisualizerInstance->GetData(CerboSSH::GetRawString());
            }
            if (AddAppControlButton("Verbindung trennen",
                                    CerboSSH::GetConnectionState() >= SSHTypes::ConnectionState::SESSION,
                                    disConnectIcon,
                                    true))
            {
                CerboSSH::DisconnectCerbo();
            }
            AddConnectionInfo(GUITypes::DataSource::SSH);
        }
        if (ImGui::CollapsingHeader("Modbus-Daten", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (AddAppControlButton("Verbindung aufbauen##2",
                                    CerboModbus::GetConnectionState() < ModbusTypes::ConnectionState::CONNECTED,
                                    connectIcon,
                                    false))
            {
                CerboModbus::Connect();
            }
            bool readingActive = CerboModbus::GetReadingActive();
            const GUITypes::IconData modbusIcon = readingActive ? pauseIcon : readIcon;
            if (readingActive)
            {
                CerboModbus::ReadAll();
            }
            if (AddAppControlButton("Daten lesen##2",
                                    CerboModbus::GetConnectionState() >= ModbusTypes::ConnectionState::CONNECTED,
                                    modbusIcon,
                                    false))
            {
                CerboModbus::ToggleReadingActive();
            }
            if (AddAppControlButton("Verbindung trennen##2",
                                    CerboModbus::GetConnectionState() >= ModbusTypes::ConnectionState::CONNECTED,
                                    disConnectIcon,
                                    true))
            {
                CerboModbus::Disconnect();
            }
            AddConnectionInfo(GUITypes::DataSource::MODBUS);
        }
        if (ImGui::CollapsingHeader("API-Daten", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (AddAppControlButton("Verbindung aufbauen##3", true, connectIcon, false))
            {
                CerboVrm::Connect();
            }
            if (AddAppControlButton("Daten lesen##3", true, readIcon, false))
            {
            }
            if (AddAppControlButton("Verbindung trennen##3", true, disConnectIcon, true))
            {
            }
            AddConnectionInfo(GUITypes::DataSource::API);
        }
        ImGuiIO& io = ImGui::GetIO();
        ImGui::TextWrapped("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    }

    void AddConnectionInfo(GUITypes::DataSource dataSource)
    {
        // TBD: POSITIONIERUNG
        SSHTypes::ConnectionState sshConnectionState;
        ModbusTypes::ConnectionState modbusConnectionState;

        const uint16_t width = 10;
        const uint16_t height = 10;
        const uint16_t padding = 2;

        ImDrawList* windowDrawList = ImGui::GetWindowDrawList();
        ImU32 rectColor;
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();

        ImVec2 topLeft{cursorPos.x, cursorPos.y};
        ImVec2 bottomRight{cursorPos.x + width, cursorPos.y + height};

        float progress;

        switch (dataSource)
        {
        case GUITypes::DataSource::SSH:
            sshConnectionState = CerboSSH::GetConnectionState();
            for (uint16_t i = 0; i < SSHTypes::ProgressLookup.at(SSHTypes::ConnectionState::READ_RESULT); i++)
            {
                rectColor = sshConnectionState > i ? ImGui::GetColorU32(ImVec4{0, 1, 0, 1})
                                                   : ImGui::GetColorU32(ImVec4{0, 1, 0, 0.2f});
                windowDrawList->AddRectFilled(topLeft, bottomRight, rectColor);
                topLeft.x += width + padding;
                bottomRight.x += width + padding;
            }
            progress = SSHDataHandler::GetConversionProgress();
            break;

        case GUITypes::DataSource::MODBUS:
            modbusConnectionState = CerboModbus::GetConnectionState();
            for (uint16_t i = 0; i < ModbusTypes::ProgressLookup.at(ModbusTypes::ConnectionState::READ_RESULT); i++)
            {
                rectColor = modbusConnectionState > i ? ImGui::GetColorU32(ImVec4{0, 1, 0, 1})
                                                      : ImGui::GetColorU32(ImVec4{0, 1, 0, 0.2f});
                windowDrawList->AddRectFilled(topLeft, bottomRight, rectColor);
                topLeft.x += width + padding;
                bottomRight.x += width + padding;
            }
            progress = 0;
            break;

        case GUITypes::DataSource::API:
            break;
        default:
            break;
        }
        ImGui::SetCursorScreenPos(ImVec2(cursorPos.x, cursorPos.y + height + ImGui::GetStyle().ItemSpacing.y));
        ImGui::ProgressBar(progress);
    }

    void AddLoggerControls()
    {
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 windowSize = ImGui::GetWindowSize();
        float x = windowPos.x + windowSize.x - 50 - ImGui::GetStyle().WindowPadding.x;
        float y = windowPos.y + windowSize.y - 50 - ImGui::GetStyle().WindowPadding.y;

        if (ImGui::GetScrollMaxY() > 0.0f)
            x -= ImGui::GetStyle().ScrollbarSize;

        ImVec2 buttonPos = ImVec2(x, y);

        ImGui::SetCursorScreenPos(buttonPos);
        if (ImGui::Button("Clear", ImVec2(50, 50)))
        {
            CerboLog::ResetLogs();
            ImGui::SetScrollY(0.0f);
        }
    }

    void AddDataControls()
    {
        const std::vector<std::string> header = SSHDataHandler::GetHeader();
        const GUITypes::IconData barIcon = IconMap.at(GUITypes::IconVariants::BAR);
        const GUITypes::IconData lineIcon = IconMap.at(GUITypes::IconVariants::LINE);

        if (ImGui::CollapsingHeader("SSH-Daten", ImGuiTreeNodeFlags_DefaultOpen))
        {
            const uint16_t dataCategories = SSHDataHandler::GetDataCategories();
            const uint16_t countSubPlots = VisualizerInstance->GetSubPlotCount();
            std::array<bool, PDTypes::noDragSourceTypes> plotInSubPlotArray;
            std::array<bool, PDTypes::noDragSourceTypes> plotInSubPlotArrayPrev;
            for (uint16_t i = 0; i < dataCategories; i++)
            {
                const std::string buttonName = SSHDataHandler::GetTopicName(i);
                if (ImGui::TreeNode(buttonName.c_str()))
                {

                    for (uint16_t j = 0; j < countSubPlots; j++)
                    {
                        plotInSubPlotArray[0] =
                            VisualizerInstance->CheckPlotInSubplot(j, i, PDTypes::DragSource::SSHRAW);
                        plotInSubPlotArray[1] =
                            VisualizerInstance->CheckPlotInSubplot(j, i, PDTypes::DragSource::SSHCOMPUTED);
                        plotInSubPlotArray[2] = VisualizerInstance->CheckPlotInSubplot(j, i, PDTypes::DragSource::API);
                        plotInSubPlotArray[3] =
                            VisualizerInstance->CheckPlotInSubplot(j, i, PDTypes::DragSource::MODBUS);
                        plotInSubPlotArrayPrev = plotInSubPlotArray;
                        if (j == 0)
                        {
                            AddButton("RAW##" + std::to_string(i), true, barIcon, 0.5);
                            BeginDragDropSource(PDTypes::DragSource::SSHRAW, i, false);
                        }

                        if (plotInSubPlotArray[0])
                        {
                            ImGui::SameLine();
                            ImGui::Checkbox(std::string{"##" + i + j}.c_str(), &plotInSubPlotArray[0]);
                        }

                        if (j == 0)
                        {
                            AddButton("LINE##" + std::to_string(i), true, lineIcon, 0.5);
                            BeginDragDropSource(PDTypes::DragSource::SSHRAW, i, false);
                        }

                        if (plotInSubPlotArray[1])
                        {
                            ImGui::SameLine();
                            ImGui::Checkbox(std::string{"##" + i + j}.c_str(), &plotInSubPlotArray[1]);
                        }

                        if (j == 0)
                        {
                            AddButton("TBD##" + std::to_string(i), true, lineIcon, 0.5);
                            BeginDragDropSource(PDTypes::DragSource::SSHRAW, i, false);
                        }

                        if (plotInSubPlotArray[2])
                        {
                            ImGui::SameLine();
                            ImGui::Checkbox(std::string{"##" + i + j}.c_str(), &plotInSubPlotArray[2]);
                        }
                        RemovePlotsFromSubplot(plotInSubPlotArray,
                                               plotInSubPlotArrayPrev,
                                               j,
                                               VisualizerInstance->GetIsInPlotInfo(i).PlotIndex,
                                               i);
                    }

                    ImGui::TreePop();
                    ImGui::Spacing();
                }
            }
        }
        if (ImGui::CollapsingHeader("API-Daten", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::TreeNode("Reserve"))
            {
                ImGui::TreePop();
                ImGui::Spacing();
            }
        }
    }

    void RemovePlotsFromSubplot(const std::array<bool, PDTypes::noDragSourceTypes>& arrayNow,
                                const std::array<bool, PDTypes::noDragSourceTypes>& arrayLastFrame,
                                uint16_t subPlotIndex,
                                uint16_t plotIndex,
                                uint16_t dataIndex)
    {
        for (uint16_t i = 0; i < PDTypes::noDragSourceTypes; i++)
        {
            if (!arrayNow[i] && arrayLastFrame[i])
            {
                VisualizerInstance->RemovePlot(subPlotIndex, plotIndex, dataIndex);
            }
        }
    }

    void BeginDragDropSource(PDTypes::DragSource dragSourceType, uint16_t i, bool mustBeInactive)
    {
        if (ImGui::IsItemActive() || !mustBeInactive)
        {
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                std::string dragSource = PDTypes::DragSourceMap.at(dragSourceType);
                PDTypes::DragDrop dragData = PDTypes::DragDrop{dragSource, i};
                VisualizerInstance->CerboPlots::Visualizer::PrepareDropTarget(dragData);
                ImGui::SetDragDropPayload(dragSource.c_str(), &dragData, sizeof(dragData));
                ImGui::EndDragDropSource();
            }
        }
    }

    void BeginWindow(const char* Name)
    {
        ImGui::Begin(Name);
        windowCount++;
    }

    void EndWindows()
    {
        for (size_t i = 0; i < windowCount; i++)
        {
            ImGui::End();
        }
    }

    void LoadIcons()
    {
        const uint16_t iconCount = GUITypes::IconLookUp.size();
        for (uint16_t i = 0; i < iconCount; i++)
        {
            std::string iconPath = "../";
            GUITypes::Icon icon = GUITypes::IconLookUp.at(i);
            IconMap.emplace(icon.IconType, nullptr);
            iconPath += icon.RelPath;
            GUITypes::IconData* iconData = &IconMap.at(icon.IconType);
            bool ret = LoadTextureFromFile(
                renderDeviceFromMain, iconPath.c_str(), &iconData->texturePtr, &iconData->width, &iconData->height);
        }
    }

  public:
    App(std::unique_ptr<CerboPlots::Visualizer> cVisualizerPtr, ID3D11Device* cRenderDeviceFromMain)
        : VisualizerInstance(std::move(cVisualizerPtr)), renderDeviceFromMain{cRenderDeviceFromMain}
    {
        windowCount = 0;
        dockSpace = 0;

        LoadIcons();
    }

    ~App() { EndWindows(); }

    uint16_t RunUI()
    {
        CreateDockSpace();
        windowCount = 0;
        BeginWindow("Steuerung");
        CreateAppControls();

        if (SSHDataHandler::ConversionPending())
        {
            VisualizerInstance->GetData(CerboSSH::GetRawString());
        }

        if (SSHDataHandler::DataAvailable())
        {
            BeginWindow("Graph");
            VisualizerInstance->PlotGraph();
        }
        BeginWindow("Datenauswahl");
        AddDataControls();

        BeginWindow("Logger");
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(100, 100));
        CerboLog::DisplayLogs();
        AddLoggerControls();
        ImGui::PopStyleVar();

        EndWindows();
        return 0;
    }
};

bool LoadTextureFromMemory(ID3D11Device* deviceFromMain,
                           const void* data,
                           size_t data_size,
                           ID3D11ShaderResourceView** out_srv,
                           int* out_width,
                           int* out_height)
{
    // Load from disk into a raw RGBA buffer
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data =
        stbi_load_from_memory((const unsigned char*)data, (int)data_size, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create texture
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = image_width;
    desc.Height = image_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    ID3D11Texture2D* pTexture = NULL;
    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = image_data;
    subResource.SysMemPitch = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;
    deviceFromMain->CreateTexture2D(&desc, &subResource, &pTexture);

    // Create texture view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    deviceFromMain->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
    pTexture->Release();

    *out_width = image_width;
    *out_height = image_height;
    stbi_image_free(image_data);

    return true;
}

bool LoadTextureFromFile(ID3D11Device* renderDeviceFromMain,
                         const char* file_name,
                         ID3D11ShaderResourceView** out_srv,
                         int* out_width,
                         int* out_height)
{
    FILE* f = fopen(file_name, "rb");
    if (f == NULL)
        return false;
    fseek(f, 0, SEEK_END);
    size_t file_size = (size_t)ftell(f);
    if (file_size == -1)
        return false;
    fseek(f, 0, SEEK_SET);
    void* file_data = IM_ALLOC(file_size);
    fread(file_data, 1, file_size, f);
    bool ret = LoadTextureFromMemory(renderDeviceFromMain, file_data, file_size, out_srv, out_width, out_height);
    IM_FREE(file_data);
    return ret;
}
} // namespace CerboGui