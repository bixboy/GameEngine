#pragma once

#include <filesystem>
#include <functional>
#include <memory>

#include "Core/Containers/String.h"
#include "Engine/Gui/Controllers/BaseAssetEditorController.h"

struct ImVec2;

namespace BixEngine::Gui
{
    class ActorEditorController final : public BaseAssetEditorController
    {
    public:
        enum class Section
        {
            Toolbar,
            Viewport,
            Outline,
            Inspector
        };

    private:
        using SharedState = BaseAssetEditorController::SharedState;

    public:
        ActorEditorController(std::shared_ptr<SharedState> sharedState, Section section);

        static std::shared_ptr<SharedState> CreateSharedState(std::filesystem::path assetPath, String stableIdRoot, std::function<void()> onCloseRequest);

    protected:
        void DrawPanelContents(GuiPanel& panel) override;
        void OnPlayRequested() override;
        void OnSaveRequested() override;
        void OnCompileRequested() override;

    private:
        void DrawToolbar();
        void DrawViewport();
        void DrawOutline();
        void DrawInspector();

        void DrawViewportGrid_(const ImVec2& size);

        Section section_;
    };
}
