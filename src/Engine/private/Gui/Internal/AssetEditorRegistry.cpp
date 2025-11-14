#include "Gui/Internal/AssetEditorRegistry.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

#include "Gui/GuiManager.h"
#include "Gui/Internal/GuiPanel.h"

namespace BixEngine::Gui
{
    void AssetEditorRegistry::Register(const std::filesystem::path& assetPath,
                                       GuiPanel& panel,
                                       GuiPanelController& controller,
                                       std::weak_ptr<BaseAssetEditorController::SharedState> sharedState)
    {
        const auto normalized = NormalizePath(assetPath);

        EditorEntry entry{};
        entry.assetPath = normalized;
        entry.panelName = panel.GetName();
        entry.panel = &panel;
        entry.controller = &controller;
        entry.sharedState = std::move(sharedState);

        editors_[normalized] = std::move(entry);
    }

    void AssetEditorRegistry::UnregisterPanel(const String& panelName)
    {
        for (auto it = editors_.begin(); it != editors_.end();)
        {
            if (it->second.panelName == panelName)
                it = editors_.erase(it);
            else
                ++it;
        }
    }

    bool AssetEditorRegistry::CloseEditor(const std::filesystem::path& assetPath)
    {
        const auto normalized = NormalizePath(assetPath);
        auto it = editors_.find(normalized);
        if (it == editors_.end())
            return false;

        EditorEntry entry = std::move(it->second);
        editors_.erase(it);

        if (auto state = entry.sharedState.lock())
            state->onCloseRequest = nullptr;

        if (guiManager_)
        {
            if (GuiPanel* panel = guiManager_->FindPanel(entry.panelName))
                panel->OnClose = nullptr;

            guiManager_->RemovePanel(entry.panelName);
        }

        return true;
    }

    GuiPanelController* AssetEditorRegistry::FindController(const std::filesystem::path& assetPath) noexcept
    {
        if (auto it = editors_.find(NormalizePath(assetPath)); it != editors_.end())
            return it->second.controller;

        return nullptr;
    }

    const AssetEditorRegistry::EditorEntry* AssetEditorRegistry::FindEntry(const std::filesystem::path& assetPath) const noexcept
    {
        if (auto it = editors_.find(NormalizePath(assetPath)); it != editors_.end())
            return &it->second;

        return nullptr;
    }

    bool AssetEditorRegistry::HasOpenEditor(const std::filesystem::path& assetPath) const noexcept
    {
        return editors_.find(NormalizePath(assetPath)) != editors_.end();
    }

    std::filesystem::path AssetEditorRegistry::NormalizePath(const std::filesystem::path& path)
    {
        if (path.empty())
            return {};

        auto normalized = path;
#if defined(_WIN32)
        normalized = normalized.make_preferred();
#endif
        return normalized.lexically_normal();
    }

    String AssetEditorRegistry::MakePanelName(const std::filesystem::path& path)
    {
        String result{"AssetEditor::"};
        const auto generic = NormalizePath(path).generic_string();
        for (char ch : generic)
        {
            if (std::isalnum(static_cast<unsigned char>(ch)))
                result += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            else
                result += '_';
        }

        if (result.IsEmpty())
            result = "AssetEditor::Asset";

        return result;
    }

    String AssetEditorRegistry::ExtractDisplayName(const std::filesystem::path& path)
    {
        if (path.empty())
            return "Asset";

        if (auto filename = path.filename().generic_string(); !filename.empty())
            return String{filename};

        if (auto stem = path.stem().generic_string(); !stem.empty())
            return String{stem};

        return String{path.generic_string()};
    }
}
