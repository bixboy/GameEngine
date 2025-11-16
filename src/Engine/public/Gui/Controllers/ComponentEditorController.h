#pragma once
#include <filesystem>
#include <functional>
#include <memory>
#include "Containers/String.h"
#include "Gui/Controllers/BaseAssetEditorController.h"


namespace BixEngine::Gui
{
    class ComponentEditorController final : public BaseAssetEditorController
    {
    public:
        enum class Section
        {
            Toolbar,
            Inspector
        };

        using SharedState = BaseAssetEditorController::SharedState;

        ComponentEditorController(std::shared_ptr<SharedState> sharedState, Section section);

        static std::shared_ptr<SharedState> CreateSharedState(std::filesystem::path assetPath, String stableIdRoot,
                                                              std::function<void()> onCloseRequest);

    protected:
        void DrawPanelContents(GuiPanel& panel) override;
        void OnPlayRequested() override;
        void OnSaveRequested() override;
        void OnCompileRequested() override;

    private:
        void DrawToolbar();
        void DrawInspector();

        Section section_;
    };
}
