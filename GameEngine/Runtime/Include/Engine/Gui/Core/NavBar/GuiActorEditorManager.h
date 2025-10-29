#pragma once

#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "Core/Containers/String.h"
#include "Engine/Gui/Controllers/ActorEditorController.h"
#include "Engine/Gui/Core/GuiLayoutManager.h"
#include "Engine/Gui/Core/GuiManager.h"

namespace BixEngine::Gui
{
    struct ActorEditorPanels
    {
        GuiPanel* toolbar = nullptr;
        GuiPanel* viewport = nullptr;
        GuiPanel* outline = nullptr;
        GuiPanel* inspector = nullptr;

        template<typename Fn>
        void ForEachPanel(Fn&& fn) const
        {
            if (toolbar) fn(toolbar);
            if (viewport) fn(viewport);
            if (outline) fn(outline);
            if (inspector) fn(inspector);
        }

        std::span<GuiPanel*> CopyTo(std::span<GuiPanel*> dest) const noexcept
        {
            size_t index = 0;
            ForEachPanel([&](GuiPanel* p){
                if (index < dest.size())
                    dest[index++] = p;
            });
            return dest.first(index);
        }
    };

    struct ActorEditorEntry
    {
        std::filesystem::path assetPath;
        std::string navigationId;
        String buttonLabel;
        std::shared_ptr<ActorEditorController::SharedState> sharedState;
        ActorEditorPanels panels;
    };

    class GuiActorEditorManager
    {
    public:
        GuiActorEditorManager(GuiManager& guiManager, GuiLayoutManager& layoutManager);

        // Gestion des onglets d'éditeurs
        void OpenActorEditor(const std::filesystem::path& path, Core::SubsystemManager* subsystems);
        void CloseActorEditor(const std::string& navigationId);

        // Rafraîchissement
        void RefreshActorPanelsVisibility();
        void ApplyActorEditorPanels(ActorEditorEntry& entry);

        // Données
        [[nodiscard]] bool HasEditors() const noexcept { return !actorEditors_.empty(); }
        [[nodiscard]] const std::string& GetActiveNavigationId() const noexcept { return activeNavigationId_; }

        void SetActiveLayout(EditorLayoutType layout) noexcept { activeLayout_ = layout; }
        [[nodiscard]] EditorLayoutType GetActiveLayout() const noexcept { return activeLayout_; }

        std::span<GuiPanel*> CollectPanels(const ActorEditorPanels& panels, std::array<GuiPanel*, 4>& buffer) const noexcept;

    private:
        GuiManager& guiManager_;
        GuiLayoutManager& layoutManager_;

        std::unordered_map<std::string, ActorEditorEntry> actorEditors_;
        std::unordered_map<std::filesystem::path, std::string> actorEditorsByPath_;
        std::vector<std::string> actorEditorOrder_;

        std::string activeNavigationId_ = "scene";
        EditorLayoutType activeLayout_ = EditorLayoutType::Scene;
    };
}
