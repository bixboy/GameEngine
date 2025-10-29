#pragma once

#include <functional>

#include "Core/Containers/String.h"
#include "Engine/Gui/Core/GuiPanel.h"

namespace BixEngine::Gui
{
    /**
     * @brief Dedicated editor panel for actor assets.
     *
     * The panel is split into three child regions:
     *  - a viewport area used to preview the actor in real-time;
     *  - a middle column to list actor data (components, hierarchy);
     *  - a right inspector column to expose editable properties.
     */
    class ActorEditorPanel : public GuiPanel
    {
    public:
        struct DrawCallbacks
        {
            std::function<void()> viewport{};
            std::function<void()> outline{};
            std::function<void()> inspector{};
        };

        struct ToolbarCallbacks
        {
            std::function<void()> onPlay{};
            std::function<void()> onSave{};
            std::function<void()> onCompile{};
        };

        ActorEditorPanel(String name, String title);

        void SetDrawCallbacks(DrawCallbacks callbacks) noexcept { callbacks_ = std::move(callbacks); }
        void SetToolbarCallbacks(ToolbarCallbacks callbacks) noexcept { toolbar_ = std::move(callbacks); }

        void DrawEditor();

    private:
        void DrawToolbar_();
        void DrawLayout_();
        void DrawViewportArea_();
        void DrawOutlineArea_();
        void DrawInspectorArea_();

        DrawCallbacks callbacks_{};
        ToolbarCallbacks toolbar_{};
        float outlineWidthRatio_{0.22f};
        float inspectorWidthRatio_{0.28f};
    };
}
