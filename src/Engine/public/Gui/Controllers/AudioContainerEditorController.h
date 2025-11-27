#pragma once
#include "Gui/Controllers/BaseAssetEditorController.h"
#include "Ressources/RessourcesClass/AudioContainer.h"

namespace BixEngine::Gui
{
    class AudioContainerEditorController final : public BaseAssetEditorController
    {
    public:
        AudioContainerEditorController(std::shared_ptr<SharedState> sharedState);
        ~AudioContainerEditorController() override;

    protected:
        void DrawPanelContents(GuiPanel& panel) override;
        void OnSaveRequested() override;

    private:
        void LoadResource();
        void DrawTracks();
        void DrawProperties();

        std::shared_ptr<resources::AudioContainer> audioContainer_;
        bool isDirty_ = false;

        struct PreviewState;
        std::unique_ptr<PreviewState> previewState_;
    };
}
