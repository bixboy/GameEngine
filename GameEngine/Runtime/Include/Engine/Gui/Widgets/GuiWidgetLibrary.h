#pragma once

#include <functional>
#include <string>
#include <vector>

#include "imgui.h"

namespace BixEngine::Gui::Widgets
{
    struct MetricDisplay
    {
        std::string label{};
        std::string value{};
        ImVec4 color{1.0f, 1.0f, 1.0f, 1.0f};
        std::string hint{};
    };

    struct PanelHeaderOptions
    {
        std::string title{};
        std::string subtitle{};
        bool showSeparator{true};
    };

    void DrawPanelHeader(const PanelHeaderOptions& options);
    void DrawMetricRow(const MetricDisplay& metric);
    void DrawMetricsTable(const std::vector<MetricDisplay>& metrics, float columnWidth = 140.0f);

    class PanelSection
    {
    public:
        explicit PanelSection(const char* label, bool defaultOpen = true, ImGuiTreeNodeFlags flags = 0);
        ~PanelSection();

        PanelSection(const PanelSection&) = delete;
        PanelSection& operator=(const PanelSection&) = delete;
        PanelSection(PanelSection&&) noexcept = delete;
        PanelSection& operator=(PanelSection&&) noexcept = delete;

        [[nodiscard]] bool IsOpen() const noexcept { return open_; }

    private:
        bool open_{false};
    };

    class PanelToolbar
    {
    public:
        PanelToolbar();
        ~PanelToolbar();

        PanelToolbar(const PanelToolbar&) = delete;
        PanelToolbar& operator=(const PanelToolbar&) = delete;
        PanelToolbar(PanelToolbar&&) noexcept = delete;
        PanelToolbar& operator=(PanelToolbar&&) noexcept = delete;

        void AddLeft(const std::function<void()>& drawCallback);
        void AddRight(const std::function<void()>& drawCallback);
        void Commit();

    private:
        std::vector<std::function<void()>> leftElements_;
        std::vector<std::function<void()>> rightElements_;
        bool committed_{false};
    };

    namespace Builder
    {
        class Section
        {
        public:
            Section(const char* label, bool defaultOpen = true, ImGuiTreeNodeFlags flags = 0);
            ~Section();

            Section(const Section&) = delete;
            Section& operator=(const Section&) = delete;
            Section(Section&&) noexcept = delete;
            Section& operator=(Section&&) noexcept = delete;

            [[nodiscard]] bool IsOpen() const noexcept { return section_.IsOpen(); }

        private:
            PanelSection section_;
        };
    }
}

