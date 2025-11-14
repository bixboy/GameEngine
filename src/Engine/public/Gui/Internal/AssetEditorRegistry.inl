#pragma once

#include "Gui/Internal/AssetEditorRegistry.h"

#include <stdexcept>

#include "Gui/GuiManager.h"
#include "Gui/Internal/GuiPanel.h"

namespace BixEngine::Gui
{
    template <typename ControllerT, typename... Args>
    ControllerT& AssetEditorRegistry::OpenEditor(const std::filesystem::path& assetPath,
                                                 BaseAssetEditorController::PanelConfig config,
                                                 Args&&... args)
    {
        const auto normalized = NormalizePath(assetPath);

        if (auto it = editors_.find(normalized); it != editors_.end())
        {
            if (guiManager_ && it->second.panel)
            {
                it->second.panel->SetVisible(true);
                it->second.panel->RequestFocus();
            }

            if (auto* controller = dynamic_cast<ControllerT*>(it->second.controller))
                return *controller;

            editors_.erase(it);
        }

        if (!guiManager_)
            throw std::runtime_error("AssetEditorRegistry::OpenEditor — aucun GuiManager disponible.");

        auto sharedState = std::make_shared<BaseAssetEditorController::SharedState>();
        sharedState->assetPath = normalized;
        sharedState->assetDisplayName = ExtractDisplayName(normalized);
        sharedState->stableIdRoot = MakePanelName(normalized);
        sharedState->assetTypeLabel = config.titlePrefix.IsEmpty() ? String{"Asset"} : config.titlePrefix;

        String panelName = sharedState->stableIdRoot;
        String title = sharedState->assetDisplayName.IsEmpty() ? String{"Asset Editor"} : sharedState->assetDisplayName;

        sharedState->onCloseRequest = [this, normalized]() mutable
        {
            CloseEditor(normalized);
        };

        ControllerT& controller = guiManager_->OpenPanel<ControllerT>(panelName, title, sharedState, std::move(config),
                                                                      std::forward<Args>(args)...);

        Register(normalized, controller.GetPanel(), controller, sharedState);

        return controller;
    }
}
