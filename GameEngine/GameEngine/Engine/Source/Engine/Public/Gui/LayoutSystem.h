#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

#include "Core/String.h"

namespace Engine::Gui
{
    class GuiPanel;

    class LayoutSystem
    {
        public:
            LayoutSystem() = default;

            void RegisterPanel(GuiPanel* panel);
            void UnregisterPanel(GuiPanel* panel);
            void Clear() noexcept;

            bool SaveLayout(const std::filesystem::path& filePath) const;
            bool LoadLayout(const std::filesystem::path& filePath);

            void ResetToDefault();
            void CaptureDefaultLayout();

            [[nodiscard]] bool HasDefaultLayout() const noexcept { return !defaultLayout_.empty(); }

        private:
            using PanelMap = std::unordered_map<String, GuiPanel*>;

            [[nodiscard]] GuiPanel* FindPanel(const String& name) const;
            void ApplyLayout(const std::string& iniData, float referenceWidth, float referenceHeight);
            void ApplyPanelVisibility(const std::unordered_map<String, bool>& visibility);

            [[nodiscard]] static std::string ScaleIniData(const std::string& iniData, float scaleX, float scaleY);
            [[nodiscard]] static bool ParseFloatPair(const std::string& value, float& outX, float& outY);
            [[nodiscard]] static std::string FormatFloat(float value);

            PanelMap panels_{};
            std::unordered_map<String, bool> defaultVisibility_{};
            std::string defaultLayout_{};
            float defaultReferenceWidth_{0.0f};
            float defaultReferenceHeight_{0.0f};
    };
}
