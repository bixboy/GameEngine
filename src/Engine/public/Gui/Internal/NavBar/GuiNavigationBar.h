#pragma once
#include <string>
#include <string_view>
#include "Gui/Internal/GuiLayoutManager.h"

struct ImVec2;

namespace BixEngine::Gui
{
    class GuiSystem;
    class GuiPanel;
    class GuiManager;
}

namespace BixEngine::Gui
{
    class GuiModule;


    class GuiNavigationBar
    {
    public:
        GuiNavigationBar(GuiSystem& guiSystem, GuiLayoutManager& layoutManager, GuiModule& owner);
        ~GuiNavigationBar() = default;

        void Render();

    private:
        bool DrawNavigationButton(const std::string& label, bool isActive, float buttonHeight) const;
        bool DrawCloseButton(std::string_view label, float buttonHeight) const;

        void DrawSceneButton(float buttonHeight);
        void DrawAssetEditorTabs(float buttonHeight, float availableWidth);
        void DrawPlayControls(float buttonHeight);
        void DrawAudioPlayer(float buttonHeight);

        GuiSystem* guiSystem_{nullptr};
        GuiLayoutManager* layoutManager_{nullptr};
        GuiModule* owner_{nullptr};

        float m_MasterVolume = 1.0f;
        bool m_IsPlaying = false;
        std::string m_CurrentSongName = "No Audio";

        std::vector<std::filesystem::path> m_CachedAudioFiles;
        std::vector<std::string> m_CachedAudioLabels;
        std::vector<std::string> m_CachedAudioPaths;
        
        float m_AudioScanTimer = 0.0f;
        bool m_AudioListDirty = true;
        
        void UpdateAudioFileList();
    };
}
