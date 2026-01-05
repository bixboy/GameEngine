#pragma once
#include <filesystem>
#include <vector>
#include <string>


namespace BixEngine::Gui
{
    class EditorSceneManager
    {
    public:
        EditorSceneManager();

        void DrawDialogs();

        void RequestNewScene();
        void RequestOpenScene();
        void RequestSaveScene();
        void RequestSaveSceneAs();
        void RequestCloseScene();
        void RequestRenameScene();
        void RequestDeleteScene();

        void LoadScene(const std::filesystem::path& path);

        [[nodiscard]] const std::vector<std::filesystem::path>& GetRecentScenes() const { return recentScenes_; }
        [[nodiscard]] const std::filesystem::path& GetCurrentScenePath() const { return currentScenePath_; }
        [[nodiscard]] bool IsSceneDirty() const { return isSceneDirty_; }
        void MarkSceneDirty(bool dirty = true) { isSceneDirty_ = dirty; }

    private:
        void DrawSaveAsDialog_();
        void DrawOpenSceneDialog_();
        void DrawDeleteSceneDialog_();
        void DrawRenameSceneDialog_();
        void DrawCloseSceneConfirmation_();
        
        void AddToRecentScenes_(const std::filesystem::path& path);
        void LoadRecentScenes_();
        void SaveRecentScenes_();

        void PerformSave_(const std::filesystem::path& path);
        void PerformLoad_(const std::filesystem::path& path);

        std::filesystem::path currentScenePath_;
        bool isSceneDirty_{false};
        
        bool showSaveAsDialog_{false};
        bool showOpenSceneDialog_{false};
        bool showDeleteSceneDialog_{false};
        bool showRenameSceneDialog_{false};
        bool showCloseSceneConfirmation_{false};

        char saveAsFilenameBuffer_[256]{};
        char renameFilenameBuffer_[256]{};
        std::string renameErrorMessage_;
        
        std::vector<std::filesystem::path> recentScenes_;
        std::filesystem::path recentScenesFile_;
    };
}
