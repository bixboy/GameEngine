#include "Engine/Gui/Core/NavBar/GuiActorEditorManager.h"

#include <algorithm>
#include <array>
#include <format>

#include "Core/Logger.h"
#include "Engine/Gui/Controllers/ActorEditorController.h"
#include "Engine/Gui/Core/GuiPanel.h"
#include "Engine/Systems/SubsystemManager.h"

namespace BixEngine::Gui
{
    namespace
    {
        std::string MakeNavigationIdFromPath(const std::filesystem::path& path)
        {
            std::string raw = path.generic_string();
            if (raw.empty())
                raw = "actor";

            std::string sanitized;
            sanitized.reserve(raw.size());
            for (char ch : raw)
            {
                if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '_')
                    sanitized.push_back(ch);
                else
                    sanitized.push_back('_');
            }

            constexpr std::uint64_t kFnvOffset = 1469598103934665603ull;
            constexpr std::uint64_t kFnvPrime = 1099511628211ull;
            std::uint64_t hash = kFnvOffset;
            for (unsigned char ch : raw)
            {
                hash ^= ch;
                hash *= kFnvPrime;
            }

            sanitized.append("_");
            sanitized.append(std::format("{:x}", hash));

            return "actor_editor_" + sanitized;
        }
    }

    GuiActorEditorManager::GuiActorEditorManager(GuiManager& guiManager, GuiLayoutManager& layoutManager)
        : guiManager_(guiManager), layoutManager_(layoutManager)
    {
    }

    // ─────────────────────────────────────────────
    // 🏗️  Ouverture d’un nouvel éditeur d’acteur
    // ─────────────────────────────────────────────
    void GuiActorEditorManager::OpenActorEditor(const std::filesystem::path& path, Core::SubsystemManager* subsystems)
    {
        if (!subsystems || path.empty())
        {
            LOG_WARNING("[GuiActorEditorManager] Invalid open request: no subsystem or empty path.");
            return;
        }

        std::filesystem::path normalized = path.lexically_normal();
        if (normalized.empty())
            normalized = path;

        // Vérifie si déjà ouvert
        auto existing = actorEditorsByPath_.find(normalized);
        if (existing != actorEditorsByPath_.end())
        {
            const std::string& navId = existing->second;
            if (auto it = actorEditors_.find(navId); it != actorEditors_.end())
            {
                ApplyActorEditorPanels(it->second);
                activeNavigationId_ = navId;
                activeLayout_ = Gui::EditorLayoutType::ActorEditor;
                layoutManager_.Switch(Gui::EditorLayoutType::ActorEditor);
                RefreshActorPanelsVisibility();
            }
            return;
        }

        // Crée un nouvel éditeur
        const std::string navigationId = MakeNavigationIdFromPath(normalized);
        const std::string baseName = navigationId;

        auto sharedState = ActorEditorController::CreateSharedState(
            *subsystems, normalized, String(navigationId.c_str()),
            [this, navigationId]() { CloseActorEditor(navigationId); });

        if (!sharedState)
        {
            LOG_ERROR("[GuiActorEditorManager] Failed to create shared state for actor editor.");
            return;
        }

        ActorEditorEntry entry{};
        entry.assetPath = normalized;
        entry.navigationId = navigationId;
        entry.buttonLabel = sharedState->assetDisplayName.Std();
        entry.sharedState = sharedState;

        auto createPanel = [&](const std::string& suffix, Gui::ActorEditorController::Section section) -> GuiPanel*
        {
            const std::string panelId = baseName + "_" + suffix;
            GuiPanel& panel = guiManager_.CreatePanel(String(panelId.c_str()), String{"Actor Editor"});
            panel.SetVisible(false);
            auto controller = std::make_unique<ActorEditorController>(sharedState, section);
            guiManager_.AttachController(panel, std::move(controller));
            return &panel;
        };

        entry.panels.toolbar = createPanel("toolbar", ActorEditorController::Section::Toolbar);
        entry.panels.viewport = createPanel("viewport", ActorEditorController::Section::Viewport);
        entry.panels.outline = createPanel("outline", ActorEditorController::Section::Outline);
        entry.panels.inspector = createPanel("inspector", ActorEditorController::Section::Inspector);

        actorEditorOrder_.push_back(navigationId);
        actorEditorsByPath_.emplace(normalized, navigationId);
        actorEditors_.emplace(navigationId, std::move(entry));

        ActorEditorEntry& storedEntry = actorEditors_.at(navigationId);

        ApplyActorEditorPanels(storedEntry);

        activeNavigationId_ = navigationId;
        layoutManager_.Switch(Gui::EditorLayoutType::ActorEditor);
        activeLayout_ = Gui::EditorLayoutType::ActorEditor;
        RefreshActorPanelsVisibility();
    }

    // ─────────────────────────────────────────────
    // 🗑️  Fermeture d’un éditeur
    // ─────────────────────────────────────────────
    void GuiActorEditorManager::CloseActorEditor(const std::string& navigationId)
    {
        auto it = actorEditors_.find(navigationId);
        if (it == actorEditors_.end())
            return;

        ActorEditorEntry entry = std::move(it->second);
        actorEditors_.erase(it);

        auto orderIt = std::find(actorEditorOrder_.begin(), actorEditorOrder_.end(), navigationId);
        if (orderIt != actorEditorOrder_.end())
            actorEditorOrder_.erase(orderIt);

        std::array<GuiPanel*, 4> buffer{};
        const auto panelSpan = CollectPanels(entry.panels, buffer);
        layoutManager_.DetachPanels(panelSpan);
        guiManager_.RemovePanels(panelSpan);

        actorEditorsByPath_.erase(entry.assetPath);
        entry.sharedState.reset();

        if (actorEditors_.empty())
        {
            activeNavigationId_ = "scene";
            layoutManager_.Switch(Gui::EditorLayoutType::Scene);
            layoutManager_.ResetLayout(Gui::EditorLayoutType::ActorEditor);
            activeLayout_ = Gui::EditorLayoutType::Scene;
        }
        else
        {
            activeNavigationId_ = actorEditorOrder_.back();
            layoutManager_.Switch(Gui::EditorLayoutType::ActorEditor);
            activeLayout_ = Gui::EditorLayoutType::ActorEditor;
        }

        RefreshActorPanelsVisibility();
    }

    // ─────────────────────────────────────────────
    // 👁️  Mise à jour visibilité
    // ─────────────────────────────────────────────
    void GuiActorEditorManager::RefreshActorPanelsVisibility()
    {
        const bool actorLayoutActive = (layoutManager_.GetCurrentLayout() == Gui::EditorLayoutType::ActorEditor) ||
                                       activeLayout_ == Gui::EditorLayoutType::ActorEditor;

        for (auto& [navId, entry] : actorEditors_)
        {
            const bool visible = actorLayoutActive && activeNavigationId_ == navId;
            entry.panels.ForEachPanel([visible](GuiPanel* panel) {
                panel->SetVisible(visible);
            });
        }
    }

    // ─────────────────────────────────────────────
    // 📦  Application / enregistrement
    // ─────────────────────────────────────────────
    void GuiActorEditorManager::ApplyActorEditorPanels(ActorEditorEntry& entry)
    {
        std::array<GuiPanel*, 4> buffer{};
        const auto span = CollectPanels(entry.panels, buffer);
        layoutManager_.RegisterPanels(Gui::EditorLayoutType::ActorEditor, span,
                                      Gui::GuiLayoutManager::LayoutRegistrationMode::LoadIfUninitialized);
    }

    // ─────────────────────────────────────────────
    // 🔧  Utilitaire : copie des panels
    // ─────────────────────────────────────────────
    std::span<GuiPanel*> GuiActorEditorManager::CollectPanels(const ActorEditorPanels& panels,
                                                              std::array<GuiPanel*, 4>& buffer) const noexcept
    {
        std::span spanBuffer{buffer};
        return panels.CopyTo(spanBuffer);
    }
}
