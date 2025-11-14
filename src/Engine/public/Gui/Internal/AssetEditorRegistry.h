#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

#include "Containers/String.h"
#include "Gui/Controllers/BaseAssetEditorController.h"


namespace BixEngine::Gui
{
    class GuiManager;
    class GuiPanel;
    class GuiPanelController;

    /**
     * @brief Gestion des éditeurs d'assets ouverts.
     */
    class AssetEditorRegistry
    {
    public:
        struct EditorEntry
        {
            std::filesystem::path assetPath;
            String panelName;
            GuiPanel* panel = nullptr;
            GuiPanelController* controller = nullptr;
            std::weak_ptr<BaseAssetEditorController::SharedState> sharedState;
        };

        AssetEditorRegistry() = default;

        void SetGuiManager(GuiManager* manager) noexcept { guiManager_ = manager; }

        void Register(const std::filesystem::path& assetPath, GuiPanel& panel, GuiPanelController& controller, std::weak_ptr<BaseAssetEditorController::SharedState> sharedState);

        void UnregisterPanel(const String& panelName);
        bool CloseEditor(const std::filesystem::path& assetPath);

        [[nodiscard]] GuiPanelController* FindController(const std::filesystem::path& assetPath) noexcept;
        [[nodiscard]] const EditorEntry* FindEntry(const std::filesystem::path& assetPath) const noexcept;
        [[nodiscard]] bool HasOpenEditor(const std::filesystem::path& assetPath) const noexcept;
    
    private:
        [[nodiscard]] EditorEntry* GetEntry(const std::filesystem::path& assetPath) noexcept;

        struct PathHash
        {
            size_t operator()(const std::filesystem::path& value) const noexcept
            {
                return std::hash<std::string>{}(value.generic_string());
            }
        };
        
        std::unordered_map<std::filesystem::path, EditorEntry, PathHash> editors_;
        GuiManager* guiManager_ = nullptr;
    };
}
