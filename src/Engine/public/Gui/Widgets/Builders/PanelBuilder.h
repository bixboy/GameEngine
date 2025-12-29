#pragma once
#include <string>

#include "Gui/Widgets/Layout/PanelToolbar.h"

namespace BixEngine::Gui::Widgets
{
     
    struct PanelHeaderOptions
    {
        std::string title{};
        std::string subtitle{};
        bool showSeparator{true};
    };

     
    void DrawPanelHeader(const PanelHeaderOptions& options);

     
    class PanelBuilder
    {
    public:
        explicit PanelBuilder(PanelHeaderOptions options);

         
        void DrawHeader() const;

         
        PanelToolbar& Toolbar() noexcept { return toolbar_; }

         
        void DrawToolbar();

    private:
        PanelHeaderOptions options_{};
        PanelToolbar toolbar_{};
    };
}
