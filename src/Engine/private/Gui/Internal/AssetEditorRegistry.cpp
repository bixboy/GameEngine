#include "Gui/Internal/AssetEditorRegistry.h"
#include "Gui/Core/GuiManager.h"
#include "Gui/Panels/GuiPanel.h"
#include <algorithm>
#include "Utils/FileIO/FilesUtils.h"


namespace BixEngine::Gui
{
    // --------------------------------------------------------------
    // REGISTER
    // --------------------------------------------------------------
    void AssetEditorRegistry::Register(const std::filesystem::path& assetPath, GuiPanel& panel, GuiPanelController& controller, std::weak_ptr<BaseAssetEditorController::SharedState> sharedState)
    {
        const auto normalized = BixEngine::FilesUtils::Utilities::NormalizePath(assetPath);

        editors_[normalized] = EditorEntry{
            .assetPath = normalized,
            .panelName = panel.GetName(),
            .panel = &panel,
            .controller = &controller,
            .sharedState = std::move(sharedState)
        };
    }

    // --------------------------------------------------------------
    // UNREGISTER BY PANEL NAME
    // --------------------------------------------------------------
    void AssetEditorRegistry::UnregisterPanel(const String& panelName)
    {
        std::erase_if(editors_, [&](const auto& kvp)
        {
            return kvp.second.panelName == panelName;
        });
    }

    // --------------------------------------------------------------
    // CLOSE EDITOR
    // --------------------------------------------------------------
    bool AssetEditorRegistry::CloseEditor(const std::filesystem::path& assetPath)
    {
        auto* entry = GetEntry(assetPath);
        if (!entry)
            return false;

        EditorEntry localCopy = *entry;
        editors_.erase(BixEngine::FilesUtils::Utilities::NormalizePath(assetPath));

        if (auto state = localCopy.sharedState.lock())
            state->onCloseRequest = nullptr;

        if (guiManager_)
        {
            if (GuiPanel* panel = guiManager_->FindPanel(localCopy.panelName))
                panel->OnClose = nullptr;

            guiManager_->RemovePanel(localCopy.panelName);
        }

        return true;
    }

    // --------------------------------------------------------------
    // FIND CONTROLLER
    // --------------------------------------------------------------
    GuiPanelController* AssetEditorRegistry::FindController(const std::filesystem::path& assetPath) noexcept
    {
        if (auto* entry = GetEntry(assetPath))
            return entry->controller;
        
        return nullptr;
    }
    
    AssetEditorRegistry::EditorEntry* AssetEditorRegistry::FindEntry(const std::filesystem::path& assetPath) noexcept
    {
        return GetEntry(assetPath);
    }

    bool AssetEditorRegistry::HasOpenEditor(const std::filesystem::path& assetPath) noexcept
    {
        return GetEntry(assetPath) != nullptr;
    }

    // --------------------------------------------------------------
    // INTERNAL ACCESSORS
    // --------------------------------------------------------------
    AssetEditorRegistry::EditorEntry* AssetEditorRegistry::GetEntry(const std::filesystem::path& assetPath) noexcept
    {
        if (auto it = editors_.find(BixEngine::FilesUtils::Utilities::NormalizePath(assetPath)); it != editors_.end())
            return &it->second;
        
        return nullptr;
    }
    
}
