#pragma once
#include <string>
#include <functional> // Nécessaire pour le callback de la toolbar

#include "Gui/Widgets/Layout/PanelToolbar.h"
#include "Gui/Widgets/ImGuiScopeBase.h"

namespace BixEngine::Gui::Widgets
{
    struct PanelHeaderOptions
    {
        std::string title{};
        std::string subtitle{};
        bool showSeparator{true};
    };

    void DrawPanelHeader(const PanelHeaderOptions& options);

    class PanelBuilder : public ImGuiScopeBase
    {
    public:
        explicit PanelBuilder(const PanelHeaderOptions& options, std::function<void(Layout::PanelToolbar&)> toolbarConfig = nullptr);
        ~PanelBuilder();

        PanelBuilder(const PanelBuilder&) = delete;
        PanelBuilder& operator=(const PanelBuilder&) = delete;
        PanelBuilder(PanelBuilder&&) = delete;
        PanelBuilder& operator=(PanelBuilder&&) = delete;

        operator bool() const { return IsActive(); }

    private:
        Layout::PanelToolbar toolbar_{};
    };
}