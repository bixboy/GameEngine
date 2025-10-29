#include "Engine/Gui/Core/NavBar/GuiActorEditorManager.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <format>
#include <string_view>
#include <utility>

#include "Core/Logger.h"
#include "Engine/Gui/Core/GuiManager.h"
#include "Engine/Gui/Core/GuiPanel.h"
#include "Engine/Systems/SubsystemManager.h"

namespace BixEngine::Core
{
    namespace
    {
        constexpr std::string_view kSceneNavigationId{"scene"};

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

    GuiActorEditorManager::GuiActorEditorManager(Gui::GuiManager& guiManager,
                                                 Gui::GuiLayoutManager* layoutManager,
                                                 FocusRequestCallback focusRequestCallback,
                                                 FocusSceneCallback focusSceneCallback)
        : guiManager_(&guiManager)
        , layoutManager_(layoutManager)
        , focusRequestCallback_(std::move(focusRequestCallback))
        , focusSceneCallback_(std::move(focusSceneCallback))
    {
    }

    void GuiActorEditorManager::SetLayoutManager(Gui::GuiLayoutManager* layoutManager) noexcept
    {
        layoutManager_ = layoutManager;
    }

    void GuiActorEditorManager::SetSubsystems(SubsystemManager* subsystems) noexcept
    {
        subsystems_ = subsystems;
    }

    void GuiActorEditorManager::SetFocusCallbacks(FocusRequestCallback focusRequestCallback,
                                                  FocusSceneCallback focusSceneCallback)
    {
        focusRequestCallback_ = std::move(focusRequestCallback);
        focusSceneCallback_ = std::move(focusSceneCallback);
    }

    void GuiActorEditorManager::OpenActorEditor(const std::filesystem::path& path)
    {
        if (!guiManager_)
            return;

        if (!subsystems_)
        {
            LOG_WARNING("[GuiActorEditorManager] Unable to open actor editor without subsystem context.");
            return;
        }

        if (path.empty())
            return;

        std::filesystem::path normalized = path.lexically_normal();
        if (normalized.empty())
            normalized = path;

        if (normalized.empty())
            return;

        if (auto existingByPath = actorEditorsByPath_.find(normalized); existingByPath != actorEditorsByPath_.end())
        {
            const std::string& navigationId = existingByPath->second;
            ActivateEditor(navigationId, true);
            return;
        }

        const std::string navigationId = MakeNavigationIdFromPath(normalized);
        const std::string baseName = navigationId;

        auto sharedState = Gui::ActorEditorController::CreateSharedState(
            *subsystems_, normalized, String(navigationId.c_str()),
            [this, navigationId]()
            {
                CloseActorEditor(navigationId);
            });

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

        auto createPanel = [&](const std::string& suffix, Gui::ActorEditorController::Section section) -> Gui::GuiPanel*
        {
            const std::string panelId = baseName + "_" + suffix;
            Gui::GuiPanel& panel = guiManager_->CreatePanel(String(panelId.c_str()), String{"Actor Editor"});
            panel.SetVisible(false);
            auto controller = std::make_unique<Gui::ActorEditorController>(sharedState, section);
            guiManager_->AttachController(panel, std::move(controller));
            return &panel;
        };

        entry.panels.toolbar = createPanel("toolbar", Gui::ActorEditorController::Section::Toolbar);
        entry.panels.viewport = createPanel("viewport", Gui::ActorEditorController::Section::Viewport);
        entry.panels.outline = createPanel("outline", Gui::ActorEditorController::Section::Outline);
        entry.panels.inspector = createPanel("inspector", Gui::ActorEditorController::Section::Inspector);

        actorEditorOrder_.push_back(navigationId);
        actorEditorsByPath_.emplace(normalized, navigationId);
        auto [itInserted, _] = actorEditors_.emplace(navigationId, std::move(entry));
        ActorEditorEntry& storedEntry = itInserted->second;

        ApplyActorEditorPanels(storedEntry);

        activeNavigationId_ = navigationId;
        activeLayout_ = Gui::EditorLayoutType::ActorEditor;
        if (layoutManager_)
            layoutManager_->Switch(Gui::EditorLayoutType::ActorEditor);

        RefreshActorPanelsVisibility();
        FocusPanel(storedEntry.panels.viewport);
    }

    void GuiActorEditorManager::CloseActorEditor(const std::string& navigationId)
    {
        auto it = actorEditors_.find(navigationId);
        if (it == actorEditors_.end())
            return;

        ActorEditorEntry entry = std::move(it->second);
        actorEditors_.erase(it);

        std::size_t removedIndex = 0;
        bool removedFromOrder = false;
        if (!actorEditorOrder_.empty())
        {
            auto orderIt = std::find(actorEditorOrder_.begin(), actorEditorOrder_.end(), navigationId);
            if (orderIt != actorEditorOrder_.end())
            {
                removedIndex = static_cast<std::size_t>(std::distance(actorEditorOrder_.begin(), orderIt));
                actorEditorOrder_.erase(orderIt);
                removedFromOrder = true;
            }
        }

        PanelBuffer buffer{};
        const auto panelSpan = CollectPanels(entry.panels, buffer);
        if (layoutManager_)
            layoutManager_->DetachPanels(panelSpan);
        if (guiManager_)
            guiManager_->RemovePanels(panelSpan);

        actorEditorsByPath_.erase(entry.assetPath);
        entry.sharedState.reset();

        Gui::GuiPanel* panelToFocus = nullptr;
        bool focusScene = false;

        if (activeNavigationId_ == navigationId)
        {
            if (actorEditors_.empty())
            {
                activeNavigationId_ = std::string{kSceneNavigationId};
                focusScene = true;
                if (layoutManager_)
                {
                    layoutManager_->Switch(Gui::EditorLayoutType::Scene);
                    layoutManager_->ResetLayout(Gui::EditorLayoutType::ActorEditor);
                }
                activeLayout_ = Gui::EditorLayoutType::Scene;
            }
            else
            {
                if (actorEditorOrder_.empty())
                {
                    activeNavigationId_ = std::string{kSceneNavigationId};
                    focusScene = true;
                    if (layoutManager_)
                    {
                        layoutManager_->Switch(Gui::EditorLayoutType::Scene);
                        layoutManager_->ResetLayout(Gui::EditorLayoutType::ActorEditor);
                    }
                    activeLayout_ = Gui::EditorLayoutType::Scene;
                }
                else
                {
                    std::size_t nextIndex = 0;
                    if (removedFromOrder && removedIndex < actorEditorOrder_.size())
                        nextIndex = removedIndex;
                    else if (!actorEditorOrder_.empty())
                        nextIndex = actorEditorOrder_.size() - 1;

                    const std::string& nextNavigationId = actorEditorOrder_[nextIndex];
                    activeNavigationId_ = nextNavigationId;
                    activeLayout_ = Gui::EditorLayoutType::ActorEditor;
                    if (layoutManager_)
                        layoutManager_->Switch(Gui::EditorLayoutType::ActorEditor);
                    if (auto nextIt = actorEditors_.find(nextNavigationId); nextIt != actorEditors_.end())
                    {
                        ApplyActorEditorPanels(nextIt->second);
                        panelToFocus = nextIt->second.panels.viewport;
                    }
                }
            }
        }

        if (actorEditors_.empty() && layoutManager_)
            layoutManager_->ResetLayout(Gui::EditorLayoutType::ActorEditor);

        RefreshActorPanelsVisibility();

        if (focusScene)
            RequestSceneFocus();
        else
            FocusPanel(panelToFocus);
    }

    void GuiActorEditorManager::ActivateEditor(const std::string& navigationId, bool requestFocus)
    {
        auto* entry = FindEditor(navigationId);
        if (!entry)
            return;

        activeNavigationId_ = navigationId;
        activeLayout_ = Gui::EditorLayoutType::ActorEditor;
        ApplyActorEditorPanels(*entry);
        if (layoutManager_)
            layoutManager_->Switch(Gui::EditorLayoutType::ActorEditor);

        RefreshActorPanelsVisibility();

        if (requestFocus)
            FocusPanel(entry->panels.viewport);
    }

    void GuiActorEditorManager::ActivateScene(bool requestFocus)
    {
        activeNavigationId_ = std::string{kSceneNavigationId};
        activeLayout_ = Gui::EditorLayoutType::Scene;
        if (layoutManager_)
            layoutManager_->Switch(Gui::EditorLayoutType::Scene);

        RefreshActorPanelsVisibility();

        if (requestFocus)
            RequestSceneFocus();
    }

    void GuiActorEditorManager::RefreshActorPanelsVisibility()
    {
        const bool actorLayoutActive =
            (layoutManager_ && layoutManager_->GetCurrentLayout() == Gui::EditorLayoutType::ActorEditor) ||
            activeLayout_ == Gui::EditorLayoutType::ActorEditor;

        for (auto& [navigationId, entry] : actorEditors_)
        {
            const bool shouldBeVisible = actorLayoutActive && activeNavigationId_ == navigationId;
            entry.panels.ForEachPanel([shouldBeVisible](Gui::GuiPanel* panel)
            {
                panel->SetVisible(shouldBeVisible);
            });
        }
    }

    void GuiActorEditorManager::RemoveAllEditors()
    {
        for (auto& [_, entry] : actorEditors_)
        {
            PanelBuffer buffer{};
            const auto panelSpan = CollectPanels(entry.panels, buffer);
            if (layoutManager_)
                layoutManager_->DetachPanels(panelSpan);
            if (guiManager_)
                guiManager_->RemovePanels(panelSpan);
            entry.sharedState.reset();
        }

        actorEditors_.clear();
        actorEditorsByPath_.clear();
        actorEditorOrder_.clear();
        activeNavigationId_ = std::string{kSceneNavigationId};
        activeLayout_ = Gui::EditorLayoutType::Scene;

        if (layoutManager_)
            layoutManager_->ResetLayout(Gui::EditorLayoutType::ActorEditor);
    }

    void GuiActorEditorManager::OnLayoutChanged(Gui::EditorLayoutType layout) noexcept
    {
        activeLayout_ = layout;
    }

    GuiActorEditorManager::ActorEditorEntry* GuiActorEditorManager::FindEditor(const std::string& navigationId) noexcept
    {
        if (auto it = actorEditors_.find(navigationId); it != actorEditors_.end())
            return &it->second;
        return nullptr;
    }

    const GuiActorEditorManager::ActorEditorEntry* GuiActorEditorManager::FindEditor(const std::string& navigationId) const noexcept
    {
        if (auto it = actorEditors_.find(navigationId); it != actorEditors_.end())
            return &it->second;
        return nullptr;
    }

    void GuiActorEditorManager::RemoveNavigationIdFromOrder(const std::string& navigationId)
    {
        auto it = std::find(actorEditorOrder_.begin(), actorEditorOrder_.end(), navigationId);
        if (it != actorEditorOrder_.end())
            actorEditorOrder_.erase(it);
    }

    void GuiActorEditorManager::ApplyActorEditorPanels(ActorEditorEntry& entry)
    {
        if (!layoutManager_)
            return;

        PanelBuffer buffer{};
        const auto panelSpan = CollectPanels(entry.panels, buffer);
        layoutManager_->RegisterPanels(Gui::EditorLayoutType::ActorEditor, panelSpan,
                                       Gui::GuiLayoutManager::LayoutRegistrationMode::LoadIfUninitialized);
    }

    std::span<Gui::GuiPanel*> GuiActorEditorManager::CollectPanels(const ActorEditorPanels& panels, PanelBuffer& buffer) const noexcept
    {
        std::span spanBuffer{buffer};
        return panels.CopyTo(spanBuffer);
    }

    void GuiActorEditorManager::FocusPanel(Gui::GuiPanel* panel) const
    {
        if (!panel || !focusRequestCallback_)
            return;

        focusRequestCallback_(panel->GetTitle().Std());
    }

    void GuiActorEditorManager::RequestSceneFocus() const
    {
        if (focusSceneCallback_)
            focusSceneCallback_();
    }
}
