#include "Gui/LayoutSystem.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string_view>
#include <system_error>

#include "Gui/GuiPanel.h"

#include "imgui.h"

namespace Engine::Gui
{
    namespace
    {
        constexpr float kScaleEpsilon = 0.001f;

        [[nodiscard]] bool IsFloatChar(char c) noexcept
        {
            return (c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.' || c == ',';
        }
    }

    void LayoutSystem::RegisterPanel(GuiPanel* panel)
    {
        if (!panel)
            return;

        panels_[panel->GetName()] = panel;
    }

    void LayoutSystem::UnregisterPanel(GuiPanel* panel)
    {
        if (!panel)
            return;

        const String& name = panel->GetName();
        if (auto it = panels_.find(name); it != panels_.end() && it->second == panel)
        {
            panels_.erase(it);
            return;
        }

        for (auto it = panels_.begin(); it != panels_.end(); ++it)
        {
            if (it->second == panel)
            {
                panels_.erase(it);
                break;
            }
        }
    }

    void LayoutSystem::Clear() noexcept
    {
        panels_.clear();
        defaultVisibility_.clear();
        defaultLayout_.clear();
        defaultReferenceWidth_ = 0.0f;
        defaultReferenceHeight_ = 0.0f;
    }

    bool LayoutSystem::SaveLayout(const std::filesystem::path& filePath) const
    {
        if (!ImGui::GetCurrentContext())
            return false;

        size_t iniSize = 0;
        const char* iniData = ImGui::SaveIniSettingsToMemory(&iniSize);
        if (!iniData || iniSize == 0)
            return false;

        const ImGuiIO& io = ImGui::GetIO();
        const float referenceWidth = io.DisplaySize.x;
        const float referenceHeight = io.DisplaySize.y;

        if (!filePath.parent_path().empty())
        {
            std::error_code ec;
            std::filesystem::create_directories(filePath.parent_path(), ec);
        }

        std::ofstream file(filePath);
        if (!file.is_open())
            return false;

        file << "# LayoutVersion 1\n";
        file << "ReferenceSize " << referenceWidth << ' ' << referenceHeight << "\n";
        size_t panelCount = 0;
        for (const auto& entry : panels_)
        {
            if (entry.second)
                ++panelCount;
        }

        file << "Panels " << panelCount << "\n";

        for (const auto& [name, panel] : panels_)
        {
            if (!panel)
                continue;

            file << "Panel " << std::quoted(name.Std()) << ' ' << (panel->IsVisible() ? 1 : 0) << "\n";
        }

        file << "[ImGui]\n";
        file.write(iniData, static_cast<std::streamsize>(iniSize));
        if (iniSize == 0 || iniData[iniSize - 1] != '\n')
            file << '\n';

        return file.good();
    }

    bool LayoutSystem::LoadLayout(const std::filesystem::path& filePath)
    {
        if (!ImGui::GetCurrentContext())
            return false;

        std::ifstream file(filePath);
        if (!file.is_open())
            return false;

        float referenceWidth = 0.0f;
        float referenceHeight = 0.0f;
        std::unordered_map<String, bool> visibility;
        std::string iniData;
        bool inIniSection = false;

        std::string line;
        while (std::getline(file, line))
        {
            if (!inIniSection)
            {
                if (line.empty())
                    continue;

                if (line[0] == '#')
                    continue;

                if (line == "[ImGui]")
                {
                    inIniSection = true;
                    continue;
                }

                std::istringstream iss(line);
                std::string token;
                iss >> token;

                if (token == "ReferenceSize")
                {
                    iss >> referenceWidth >> referenceHeight;
                }
                else if (token == "Panel")
                {
                    std::string name;
                    iss >> std::quoted(name);
                    int visible = 1;
                    iss >> visible;
                    visibility[String{name}] = visible != 0;
                }

                continue;
            }

            iniData.append(line);
            iniData.push_back('\n');
        }

        if (iniData.empty())
            return false;

        ApplyLayout(iniData, referenceWidth, referenceHeight);
        ApplyPanelVisibility(visibility);
        return true;
    }

    void LayoutSystem::ResetToDefault()
    {
        if (defaultLayout_.empty())
            return;

        ApplyLayout(defaultLayout_, defaultReferenceWidth_, defaultReferenceHeight_);
        ApplyPanelVisibility(defaultVisibility_);
    }

    void LayoutSystem::CaptureDefaultLayout()
    {
        if (!ImGui::GetCurrentContext())
            return;

        size_t iniSize = 0;
        const char* iniData = ImGui::SaveIniSettingsToMemory(&iniSize);
        if (!iniData || iniSize == 0)
            return;

        const ImGuiIO& io = ImGui::GetIO();
        defaultReferenceWidth_ = io.DisplaySize.x;
        defaultReferenceHeight_ = io.DisplaySize.y;
        defaultLayout_.assign(iniData, iniSize);

        defaultVisibility_.clear();
        for (const auto& [name, panel] : panels_)
        {
            if (panel)
                defaultVisibility_[name] = panel->IsVisible();
        }
    }

    GuiPanel* LayoutSystem::FindPanel(const String& name) const
    {
        if (auto it = panels_.find(name); it != panels_.end())
            return it->second;

        return nullptr;
    }

    void LayoutSystem::ApplyLayout(const std::string& iniData, float referenceWidth, float referenceHeight)
    {
        if (!ImGui::GetCurrentContext() || iniData.empty())
            return;

        const ImGuiIO& io = ImGui::GetIO();
        const float width = io.DisplaySize.x;
        const float height = io.DisplaySize.y;

        const float scaleX = (referenceWidth > 0.0f) ? width / referenceWidth : 1.0f;
        const float scaleY = (referenceHeight > 0.0f) ? height / referenceHeight : 1.0f;

        const std::string scaledIni = ScaleIniData(iniData, scaleX, scaleY);
        ImGui::LoadIniSettingsFromMemory(scaledIni.c_str(), scaledIni.size());
    }

    void LayoutSystem::ApplyPanelVisibility(const std::unordered_map<String, bool>& visibility)
    {
        for (const auto& [name, visible] : visibility)
        {
            if (GuiPanel* panel = FindPanel(name))
                panel->SetVisible(visible);
        }
    }

    std::string LayoutSystem::ScaleIniData(const std::string& iniData, float scaleX, float scaleY)
    {
        if (iniData.empty())
            return {};

        if (std::abs(scaleX - 1.0f) < kScaleEpsilon && std::abs(scaleY - 1.0f) < kScaleEpsilon)
            return iniData;

        std::string result = iniData;
        const std::array<std::string_view, 8> tokens{
            "Pos=",
            "Size=",
            "SizeRef=",
            "CentralNodeSize=",
            "ViewportPos=",
            "ViewportSize=",
            "WorkSize=",
            "WorkPos="
        };

        for (std::string_view token : tokens)
        {
            size_t searchPos = 0;
            while (searchPos < result.size())
            {
                const size_t pos = result.find(token, searchPos);
                if (pos == std::string::npos)
                    break;

                const size_t valuePos = pos + token.size();
                size_t endPos = valuePos;
                while (endPos < result.size() && IsFloatChar(result[endPos]))
                    ++endPos;

                if (endPos <= valuePos)
                {
                    searchPos = pos + token.size();
                    continue;
                }

                std::string value = result.substr(valuePos, endPos - valuePos);
                float x = 0.0f;
                float y = 0.0f;

                if (ParseFloatPair(value, x, y))
                {
                    x *= scaleX;
                    y *= scaleY;

                    const std::string replacement = FormatFloat(x) + ',' + FormatFloat(y);
                    result.replace(valuePos, value.size(), replacement);
                    searchPos = valuePos + replacement.size();
                }
                else
                {
                    searchPos = pos + token.size();
                }
            }
        }

        return result;
    }

    bool LayoutSystem::ParseFloatPair(const std::string& value, float& outX, float& outY)
    {
        const char* data = value.c_str();
        while (*data && std::isspace(static_cast<unsigned char>(*data)))
            ++data;

        char* endPtr = nullptr;
        outX = std::strtof(data, &endPtr);
        if (endPtr == data)
            return false;

        while (*endPtr && std::isspace(static_cast<unsigned char>(*endPtr)))
            ++endPtr;

        if (*endPtr != ',')
            return false;

        ++endPtr;
        while (*endPtr && std::isspace(static_cast<unsigned char>(*endPtr)))
            ++endPtr;

        char* finalPtr = nullptr;
        outY = std::strtof(endPtr, &finalPtr);
        if (finalPtr == endPtr)
            return false;

        return true;
    }

    std::string LayoutSystem::FormatFloat(float value)
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(3) << value;
        std::string result = oss.str();

        while (!result.empty() && result.back() == '0')
            result.pop_back();

        if (!result.empty() && result.back() == '.')
            result.pop_back();

        if (result.empty())
            result = "0";

        return result;
    }
}
