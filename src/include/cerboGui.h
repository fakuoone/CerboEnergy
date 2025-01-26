#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#include "cerboLogger.h"
#include "cerboModbus.h"
#include "cerboPlots.h"
#include "cerboSSH.h"
#include "dataHandler.h"
#include "dataTypes.h"
#include "imgui.h"
#include "stb_image.h"
#include "vrmApi.h"

#include <d3d11.h>
#include <array>
#include <format>
#include <iomanip>
#include <iostream>
#include <sstream>

#define ICON_SIZE 32;

namespace CerboGui {
bool LoadTextureFromMemory(ID3D11Device* deviceFromMain, const void* data, size_t data_size, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height);
bool LoadTextureFromFile(ID3D11Device* renderDeviceFromMain, const char* file_name, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height);

class App {
   private:
    uint16_t windowCount;
    ImGuiID dockSpace;
    std::unique_ptr<CerboPlots::Visualizer> VisualizerInstance;
    std::map<GUITypes::IconVariants, GUITypes::IconData> IconMap;
    ID3D11Device* renderDeviceFromMain = nullptr;

    float IconSize = 50;

    void CreateDockSpace() {
        dockSpace = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
        ImGui::SetNextWindowDockID(dockSpace);
    }

    bool AddAppControlButton(const std::string& name, const bool enable, GUITypes::IconData icon, const bool isLastInRow, const float width, const float height, float& maxWindowWidth) {
        if (AddButton(name, enable, icon, width, height)) {
            return true;
        }
        if (!isLastInRow) {
            ImGui::SameLine();
        }
        maxWindowWidth = ImGui::GetCursorPosX();
        return false;
    }

    bool AddButton(const std::string& name, const bool enable, GUITypes::IconData icon, float width, float height) {
        ImGui::BeginDisabled(!enable);
        width = width <= 2 ? (float)IconSize * width : width;      // use width both as scaling and abolute value
        height = height <= 2 ? (float)IconSize * height : height;  // use height both as scaling and abolute value

        if (ImGui::ImageButton(name.c_str(), (ImTextureID)(intptr_t)icon.texturePtr, ImVec2(width, height))) {
            ImGui::EndDisabled();
            return true;
        }
        ImGui::EndDisabled();
        return false;
    }

    void AddAppControls() {
        GUITypes::IconData connectIcon = IconMap.at(GUITypes::IconVariants::CONNECT);
        GUITypes::IconData disConnectIcon = IconMap.at(GUITypes::IconVariants::DISCONNECT);
        GUITypes::IconData readIcon = IconMap.at(GUITypes::IconVariants::READ_DATA);
        GUITypes::IconData pauseIcon = IconMap.at(GUITypes::IconVariants::PAUSE);
        float maxWidth{0};
        bool someContentExpanded{false};
        if (ImGui::CollapsingHeader("SSH-Daten", ImGuiTreeNodeFlags_DefaultOpen)) {
            someContentExpanded = true;
            if (ImGui::BeginTable("tableSSH", 3, 0)) {
                ImGui::TableNextColumn();
                if (AddAppControlButton("Verbindung aufbauen", CerboSSH::GetConnectionState() < SSHTypes::ConnectionState::CONNECTED, connectIcon, false, 1, 1, maxWidth)) {
                    CerboSSH::ConnectCerbo();
                }
                ImGui::TableNextColumn();
                if (AddAppControlButton("Daten lesen", CerboSSH::GetConnectionState() >= SSHTypes::ConnectionState::CONNECTED, readIcon, false, 1, 1, maxWidth)) {
                    SSHDataHandler::ResetSSHData();
                    CerboSSH::ReadEnergyFile();
                    VisualizerInstance->GetData(CerboSSH::GetRawString());
                }
                ImGui::TableNextColumn();
                if (AddAppControlButton("Verbindung trennen", CerboSSH::GetConnectionState() >= SSHTypes::ConnectionState::SESSION, disConnectIcon, true, 1, 1, maxWidth)) {
                    CerboSSH::DisconnectCerbo();
                }
                ImGui::EndTable();
            }
            AddConnectionInfo(GUITypes::DataSource::SSH);
        }
        if (ImGui::CollapsingHeader("Modbus-Daten", ImGuiTreeNodeFlags_DefaultOpen)) {
            someContentExpanded = true;
            if (ImGui::BeginTable("tableModbus", 3, 0)) {
                ImGui::TableNextColumn();
                if (AddAppControlButton("Verbindung aufbauen##2", CerboModbus::GetConnectionState() < ModbusTypes::ConnectionState::CONNECTED, connectIcon, false, 1, 1, maxWidth)) {
                    CerboModbus::Connect();
                }
                bool readingActive = CerboModbus::GetReadingActive();
                const GUITypes::IconData modbusIcon = readingActive ? pauseIcon : readIcon;
                ImGui::TableNextColumn();
                if (AddAppControlButton("Daten lesen##2", CerboModbus::GetConnectionState() >= ModbusTypes::ConnectionState::CONNECTED, modbusIcon, false, 1, 1, maxWidth)) {
                    CerboModbus::ToggleReadingActive();
                }
                ImGui::TableNextColumn();
                if (AddAppControlButton("Verbindung trennen##2", CerboModbus::GetConnectionState() >= ModbusTypes::ConnectionState::CONNECTED, disConnectIcon, false, 1, 1, maxWidth)) {
                    CerboModbus::Disconnect();
                }
                ImGui::EndTable();
            }
            AddConnectionInfo(GUITypes::DataSource::MODBUS);
        }
        if (ImGui::CollapsingHeader("API-Daten", ImGuiTreeNodeFlags_DefaultOpen)) {
            someContentExpanded = true;
            if (ImGui::BeginTable("tableAPI", 3, 0)) {
                ImGui::TableNextColumn();
                if (AddAppControlButton("Verbindung aufbauen##3", true, connectIcon, false, 1, 1, maxWidth)) {
                    CerboVrm::Connect();
                }
                ImGui::TableNextColumn();
                if (AddAppControlButton("Daten lesen##3", true, readIcon, false, 1, 1, maxWidth)) {}
                ImGui::TableNextColumn();
                if (AddAppControlButton("Verbindung trennen##3", true, disConnectIcon, false, 1, 1, maxWidth)) {}
                ImGui::EndTable();
            }
            AddConnectionInfo(GUITypes::DataSource::API);
        }
        ImGuiID dockNodeID = ImGui::GetWindowDockID();
        if (dockNodeID != 0 && someContentExpanded) {
            float height = ImGui::DockBuilderGetNode(dockNodeID)->Size.y;
            ImGui::DockBuilderSetNodeSize(dockNodeID, ImVec2(maxWidth, height));
        }
        ImGuiIO& io = ImGui::GetIO();
        ImGui::Text("%.1f FPS", 1000.0f / io.Framerate, io.Framerate);
    }

    void AddConnectionInfo(GUITypes::DataSource dataSource) {
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

        switch (dataSource) {
            case GUITypes::DataSource::SSH:
                sshConnectionState = CerboSSH::GetConnectionState();
                for (uint16_t i = 0; i < SSHTypes::ProgressLookup.at(SSHTypes::ConnectionState::READ_RESULT); i++) {
                    rectColor = sshConnectionState > i ? ImGui::GetColorU32(ImVec4{0, 1, 0, 1}) : ImGui::GetColorU32(ImVec4{0, 1, 0, 0.2f});
                    windowDrawList->AddRectFilled(topLeft, bottomRight, rectColor);
                    topLeft.x += width + padding;
                    bottomRight.x += width + padding;
                }
                progress = SSHDataHandler::GetConversionProgress();
                break;

            case GUITypes::DataSource::MODBUS:
                modbusConnectionState = CerboModbus::GetConnectionState();
                for (uint16_t i = 0; i < ModbusTypes::ProgressLookup.at(ModbusTypes::ConnectionState::READ_RESULT); i++) {
                    rectColor = modbusConnectionState > i ? ImGui::GetColorU32(ImVec4{0, 1, 0, 1}) : ImGui::GetColorU32(ImVec4{0, 1, 0, 0.2f});
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

    void DisplayLogs(std::vector<LogTypes::Log>& logs) {
        for (auto it = logs.crbegin(); it != logs.crend(); ++it) {
            const auto current_log = *it;

            ImGui::Text(current_log.Timestamp.c_str());
            ImGui::SameLine();

            if (current_log.Category == LogTypes::Categories::SUCCESS) {
                ImGui::Text("SUCC");
            } else if (current_log.Category == LogTypes::Categories::FAILURE) {
                ImGui::Text("FAIL");
            } else {
                assert(current_log.Category == LogTypes::Categories::INFORMATION);
                ImGui::Text("INFO");
            }
            ImGui::SameLine();
            ImGui::TextWrapped(current_log.Message.c_str());
        }
    }

    void AddLoggerControls() {
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 windowSize = ImGui::GetWindowSize();
        float x = windowPos.x + windowSize.x - 50 - ImGui::GetStyle().WindowPadding.x;
        float y = windowPos.y + windowSize.y - 50 - ImGui::GetStyle().WindowPadding.y;

        if (ImGui::GetScrollMaxY() > 0.0f)
            x -= ImGui::GetStyle().ScrollbarSize;

        ImVec2 buttonPos = ImVec2(x, y);

        ImGui::SetCursorScreenPos(buttonPos);
        if (ImGui::Button("Clear", ImVec2(50, 50))) {
            CerboLog::ResetLogs();
            ImGui::SetScrollY(0.0f);
        }
    }

    void AddRealTimeUnitData(const ModbusUnit& targetUnit) {
        const std::map<uint16_t, Register>& targetRegisters = targetUnit.GetRegisters();
        ImGui::Text(targetUnit.name.c_str());
        for (const auto& [lId, lRegister] : targetRegisters) {
            ModbusTypes::RegisterResult lResult = lRegister.GetResult();
            std::string registerText = std::format("{}: {} {}", lRegister.Name, lResult.Value, DataUnitLookup.at(lRegister.Unit));
            ImGui::Text(registerText.c_str());
        }
        const ImVec2 spacing = ImVec2{ImGui::GetCursorPos().x, ImGui::GetCursorPos().y + 2 * ImGui::GetStyle().ItemSpacing.y};
        ImGui::SetCursorPos(spacing);
    }

    void PlotRealTimeData() {
        if (!CerboModbus::GetUnitsAreCreated()) {
            return;
        }
        if (ImGui::BeginTabBar("ImPlotDemoTabs")) {
            if (ImGui::BeginTabItem("System##")) {
                AddRealTimeUnitData(CerboModbus::GetUnit(ModbusTypes::Devices::SYSTEM));
                ImGui::PushClipRect(ImGui::GetCursorScreenPos(), ImGui::GetContentRegionMax(), true);
                VisualizerInstance->PlotRealTimeData(ModbusTypes::Devices::SYSTEM);
                ImGui::PopClipRect();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Akku##0")) {
                AddRealTimeUnitData(CerboModbus::GetUnit(ModbusTypes::Devices::BATTERY));
                VisualizerInstance->PlotRealTimeData(ModbusTypes::Devices::BATTERY);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("VEBus##0")) {
                AddRealTimeUnitData(CerboModbus::GetUnit(ModbusTypes::Devices::VEBUS));
                VisualizerInstance->PlotRealTimeData(ModbusTypes::Devices::VEBUS);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::ShowDemoWindow();
        ImGui::ShowMetricsWindow();
    }

    void AddDataControls() {
        bool someContentExpanded{false};
        const GUITypes::IconData barIcon = IconMap.at(GUITypes::IconVariants::BAR);
        const GUITypes::IconData lineIcon = IconMap.at(GUITypes::IconVariants::LINE);

        if (ImGui::CollapsingHeader("SSH-Daten", ImGuiTreeNodeFlags_DefaultOpen)) {
            const uint16_t dataCategories = SSHDataHandler::GetDataCategories();
            const uint16_t countSubPlots = VisualizerInstance->GetSubPlotCount();
            for (uint16_t i = 0; i < dataCategories; i++) {
                const std::string buttonName = SSHDataHandler::GetTopicName(i);
                if (ImGui::TreeNode(buttonName.c_str())) {
                    someContentExpanded = true;
                    if (AddButton("RAW##" + std::to_string(i), true, barIcon, 0.5, 0.5))
                        VisualizerInstance->TogglePlot(i, PDTypes::DragSource::SSHRAW);
                    ImGui::SameLine();
                    if (AddButton("LINE##" + std::to_string(i), true, lineIcon, 0.5, 0.5))
                        VisualizerInstance->TogglePlot(i, PDTypes::DragSource::SSHCOMPUTED);
                    ImGui::TreePop();
                    ImGui::Spacing();
                }
            }
        }
        if (ImGui::CollapsingHeader("API-Daten", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::TreeNode("Reserve")) {
                someContentExpanded = true;
                ImGui::TreePop();
                ImGui::Spacing();
            }
        }
        ImGuiID dockNodeID = ImGui::GetWindowDockID();
        if (dockNodeID != 0 && someContentExpanded) {
            float height = ImGui::DockBuilderGetNode(dockNodeID)->Size.y;
            ImGui::DockBuilderSetNodeSize(dockNodeID, ImVec2(200, height));
        }
    }

    void BeginWindow(const char* Name, ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_None) {
        ImGui::Begin(Name, nullptr, WindowFlags);
        windowCount++;
    }

    void EndWindows() {
        for (size_t i = 0; i < windowCount; i++) {
            ImGui::End();
        }
    }

    void LoadIcons() {
        const uint16_t iconCount = GUITypes::IconLookUp.size();
        for (uint16_t i = 0; i < iconCount; i++) {
            // Shouldnt be a relative path, needs to be user config (or hard-baked into the code)
            std::string iconPath = "../";
            GUITypes::Icon icon = GUITypes::IconLookUp.at(i);
            IconMap.emplace(icon.IconType, nullptr);
            iconPath += icon.RelPath;
            GUITypes::IconData* iconData = &IconMap.at(icon.IconType);
            bool ret = LoadTextureFromFile(renderDeviceFromMain, iconPath.c_str(), &iconData->texturePtr, &iconData->width, &iconData->height);
        }
    }

   public:
    App(std::unique_ptr<CerboPlots::Visualizer> cVisualizerPtr, ID3D11Device* cRenderDeviceFromMain) : VisualizerInstance(std::move(cVisualizerPtr)), renderDeviceFromMain{cRenderDeviceFromMain} {
        windowCount = 0;
        dockSpace = 0;

        LoadIcons();
    }

    ~App() { EndWindows(); }

    uint16_t RunUI() {
        CreateDockSpace();
        windowCount = 0;

        ImGuiWindowFlags mainFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        mainFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBackground;
        BeginWindow("App", mainFlags);

        BeginWindow("Steuerung");
        AddAppControls();
        VisualizerInstance->GetData(CerboSSH::GetRawString());

        BeginWindow("Graph");
        VisualizerInstance->PlotGraph();

        BeginWindow("Echtzeitdaten");
        PlotRealTimeData();

        BeginWindow("Datenauswahl", ImGuiWindowFlags_NoResize);
        AddDataControls();

        BeginWindow("Logger");

        DisplayLogs(CerboLog::GetLogs());
        AddLoggerControls();

        EndWindows();
        return 0;
    }
};

bool LoadTextureFromMemory(ID3D11Device* deviceFromMain, const void* data, size_t data_size, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height) {
    // Load from disk into a raw RGBA buffer
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load_from_memory((const unsigned char*)data, (int)data_size, &image_width, &image_height, NULL, 4);
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

bool LoadTextureFromFile(ID3D11Device* renderDeviceFromMain, const char* file_name, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height) {
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
}  // namespace CerboGui