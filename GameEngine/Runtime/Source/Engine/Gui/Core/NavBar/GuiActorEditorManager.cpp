#include "Engine/Gui/Core/NavBar/GuiActorEditorManager.h"

#include <algorithm>
#include <array>
#include <format>
#include <ranges>
#include <string_view>

#include "Core/Logger.h"
#include "Engine/Gui/Core/GuiManager.h"
#include "Engine/Gui/Core/GuiPanel.h"
#include "Engine/Systems/SubsystemManager.h"
#include "Engine/Utils/EditorUtils.h"

namespace BixEngine::Core
{
    namespace
    {
        constexpr std::string_view kSceneNavigationId{"scene"};
    }


    GuiActorEditorManager::GuiActorEditorManager(Gui::GuiManager& guiManager, Gui::GuiLayoutManager* layoutManager, FocusRequestCallback focusRequestCallback, FocusSceneCallback focusSceneCallback)
    : guiManager_(&guiManager), layoutManager_(layoutManager), focusRequestCallback_(std::move(focusRequestCallback)), focusSceneCallback_(std::move(focusSceneCallback))
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

    void GuiActorEditorManager::SetFocusCallbacks(FocusRequestCallback focusRequestCallback, FocusSceneCallback focusSceneCallback)
    {
        focusRequestCallback_ = std::move(focusRequestCallback);
        focusSceneCallback_ = std::move(focusSceneCallback);
    }
    
    void GuiActorEditorManager::SwitchToLayout(Gui::EditorLayoutType layout, std::string_view navId, Gui::GuiPanel* panelToFocus)
    {
        activeNavigationId_ = std::string(navId);
        activeLayout_ = layout;

        if (layoutManager_)
            layoutManager_->Switch(layout);

        RefreshActorPanelsVisibility();

        if (layout == Gui::EditorLayoutType::ActorEditor)
            FocusPanel(panelToFocus);
        
        else if (layout == Gui::EditorLayoutType::Scene)
            RequestSceneFocus();
    }
    
    void GuiActorEditorManager::OpenActorEditor(const std::filesystem::path& path)
    {
        if (!guiManager_ || !subsystems_ || path.empty())
            return;

        std::filesystem::path normalized = path.lexically_normal();
        if (normalized.empty())
            normalized = path;
        
        if (normalized.empty())
            return;

        if (auto it = actorEditorsByPath_.find(normalized); it != actorEditorsByPath_.end())
        {
            ActivateEditor(it->second, true);
            return;
        }

        const std::string navigationId = std::format("actor_editor_{}_{}",  normalized.filename().string(), std::format("{:x}", Utils::HashFNV1a(normalized.generic_string())));

        auto sharedState = Gui::ActorEditorController::CreateSharedState(*subsystems_, normalized, String(navigationId.c_str()),
        [this, navigationId]()
        {
            CloseActorEditor(navigationId);
        });

        if (!sharedState)
        {
            LOG_ERROR("[GuiActorEditorManager] Failed to create shared state for actor editor.");
            return;
        }

        ActorEditorEntry entry
        {
            .assetPath = normalized,
            .navigationId = navigationId,
            .buttonLabel = sharedState->assetDisplayName.Std(),
            .sharedState = sharedState
        };

        auto makePanel = [&](std::string_view suffix, Gui::ActorEditorController::Section section)
        {
            const std::string panelId = std::format("{}_{}", navigationId, suffix);
            Gui::GuiPanel& panel = guiManager_->CreatePanel(String(panelId.c_str()), String{"Actor Editor"});
            
            panel.SetVisible(false);
            guiManager_->AttachController(panel, std::make_unique<Gui::ActorEditorController>(sharedState, section));
            
            return &panel;
        };

        entry.panels.toolbar  = makePanel("toolbar",  Gui::ActorEditorController::Section::Toolbar);
        entry.panels.viewport = makePanel("viewport", Gui::ActorEditorController::Section::Viewport);
        entry.panels.outline  = makePanel("outline",  Gui::ActorEditorController::Section::Outline);
        entry.panels.inspector= makePanel("inspector",Gui::ActorEditorController::Section::Inspector);

        actorEditorsByPath_[normalized] = navigationId;
        actorEditors_[navigationId] = std::move(entry);
        actorEditorOrder_.push_back(navigationId);

        ApplyActorEditorPanels(actorEditors_.at(navigationId));
        SwitchToLayout(Gui::EditorLayoutType::ActorEditor, navigationId, actorEditors_.at(navigationId).panels.viewport);
    }
    
    void GuiActorEditorManager::CloseActorEditor(const std::string& navigationId)
    {
        auto it = actorEditors_.find(navigationId);
        if (it == actorEditors_.end())
            return;

        ActorEditorEntry entry = std::move(it->second);
        actorEditors_.erase(it);
        actorEditorsByPath_.erase(entry.assetPath);

        DetachAndRemovePanels(entry.panels);
        entry.sharedState.reset();

        auto orderIt = std::find(actorEditorOrder_.begin(), actorEditorOrder_.end(), navigationId);
        if (orderIt != actorEditorOrder_.end())
            actorEditorOrder_.erase(orderIt);

        if (actorEditors_.empty())
        {
            if (layoutManager_)
                layoutManager_->ResetLayout(Gui::EditorLayoutType::ActorEditor);
            
            SwitchToLayout(Gui::EditorLayoutType::Scene, kSceneNavigationId);
        }
        else
        {
            const std::string& nextNav = actorEditorOrder_.back();
            ActivateEditor(nextNav, true);
        }
    }
    
    void GuiActorEditorManager::ActivateEditor(std::string_view navigationId, bool focus)
    {
        auto* entry = FindEditor(navigationId);
        if (!entry)
            return;

        ApplyActorEditorPanels(*entry);
        SwitchToLayout(Gui::EditorLayoutType::ActorEditor, navigationId, focus ? entry->panels.viewport : nullptr);
    }
    
    void GuiActorEditorManager::ActivateScene(bool focus)
    {
        SwitchToLayout(Gui::EditorLayoutType::Scene, kSceneNavigationId, focus ? nullptr : nullptr);
    }
    
    void GuiActorEditorManager::DetachAndRemovePanels(const ActorEditorPanels& panels)
    {
        PanelBuffer buffer{};
        const auto span = CollectPanels(panels, buffer);
        if (layoutManager_)
            layoutManager_->DetachPanels(span);
        
        if (guiManager_)
            guiManager_->RemovePanels(span);
    }

    void GuiActorEditorManager::RefreshActorPanelsVisibility()
    {
        const bool actorLayoutActive = (layoutManager_ && layoutManager_->GetCurrentLayout() == Gui::EditorLayoutType::ActorEditor) || activeLayout_ == Gui::EditorLayoutType::ActorEditor;

        for (auto& [navId, entry] : actorEditors_)
        {
            const bool visible = actorLayoutActive && activeNavigationId_ == navId;
            entry.panels.ForEachPanel([visible](Gui::GuiPanel* panel) {
                panel->SetVisible(visible);
            });
        }
    }

    void GuiActorEditorManager::RemoveAllEditors()
    {
        for (auto& entry : actorEditors_ | std::views::values)
        {
            DetachAndRemovePanels(entry.panels);
            entry.sharedState.reset();
        }

        actorEditors_.clear();
        actorEditorsByPath_.clear();
        actorEditorOrder_.clear();

        if (layoutManager_)
            layoutManager_->ResetLayout(Gui::EditorLayoutType::ActorEditor);

        SwitchToLayout(Gui::EditorLayoutType::Scene, kSceneNavigationId);
    }

    void GuiActorEditorManager::OnLayoutChanged(Gui::EditorLayoutType layout) noexcept
    {
        activeLayout_ = layout;
    }

    GuiActorEditorManager::ActorEditorEntry* GuiActorEditorManager::FindEditor(std::string_view navigationId) noexcept
    {
        if (auto it = actorEditors_.find(std::string(navigationId)); it != actorEditors_.end())
            return &it->second;
        
        return nullptr;
    }

    void GuiActorEditorManager::ApplyActorEditorPanels(ActorEditorEntry& entry)
    {
        if (!layoutManager_)
            return;

        PanelBuffer buffer{};
        const auto span = CollectPanels(entry.panels, buffer);
        layoutManager_->RegisterPanels(Gui::EditorLayoutType::ActorEditor, span, Gui::GuiLayoutManager::LayoutRegistrationMode::LoadIfUninitialized);
    }

    std::span<Gui::GuiPanel*> GuiActorEditorManager::CollectPanels(const ActorEditorPanels& panels, PanelBuffer& buffer) const noexcept
    {
        std::span spanBuffer{buffer};
        return panels.CopyTo(spanBuffer);
    }

    void GuiActorEditorManager::FocusPanel(Gui::GuiPanel* panel) const
    {
        if (panel && focusRequestCallback_)
            focusRequestCallback_(panel->GetTitle().Std());
    }

    void GuiActorEditorManager::RequestSceneFocus() const
    {
        if (focusSceneCallback_)
            focusSceneCallback_();
    }
}
