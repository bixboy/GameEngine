#pragma once
#include <vector>
#include <filesystem>
#include "Gui/Controllers/BaseAssetEditorWindow.h"
#include "Ressources/RessourcesClass/AudioContainer.h"


namespace BixEngine::Gui
{
    class AudioContainerEditorWindow final : public BaseAssetEditorWindow
    {
    public:
        struct AudioEditorSharedState : SharedState
        {
            std::shared_ptr<Resources::AudioContainer> container;
            std::vector<std::filesystem::path> cachedAudioFiles;
            std::filesystem::path audioRoot;
        };

        AudioContainerEditorWindow(std::shared_ptr<SharedState> sharedState);
        ~AudioContainerEditorWindow() override;
        
        AudioContainerEditorWindow(const AudioContainerEditorWindow&) = delete;
        AudioContainerEditorWindow& operator=(const AudioContainerEditorWindow&) = delete;
        
        AudioContainerEditorWindow(AudioContainerEditorWindow&&) = delete;
        AudioContainerEditorWindow& operator=(AudioContainerEditorWindow&&) = delete;
        // ------------------------------------------------

        static std::shared_ptr<SharedState> CreateSharedState(std::filesystem::path assetPath, const String& stableId, std::function<void()> onCloseRequest);

    protected:
        void DrawPanelContents(GuiPanel& panel) override;
        void OnSaveRequested() override;

    private:
        void DrawTracks();
        void DrawProperties();
        void RefreshAudioFilesCache();

        struct PreviewState; 
        std::unique_ptr<PreviewState> previewState_;
    };
}