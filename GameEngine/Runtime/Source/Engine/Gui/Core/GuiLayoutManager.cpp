#include "Engine/Gui/Core/GuiLayoutManager.h"

#include <algorithm>
#include <unordered_set>

#include "Engine/Gui/Core/GuiManager.h"
#include "Engine/Gui/Core/GuiPanel.h"
#include "Engine/Gui/Core/GuiSystem.h"

namespace BixEngine::Gui
{
    namespace
    {
        constexpr const char* kRootDockspaceWindow = "EditorRootDockspace";
    }

    GuiLayoutManager::GuiLayoutManager(GuiSystem& guiSystem, GuiManager& guiManager)
        : guiSystem_(&guiSystem), guiManager_(&guiManager)
    {
        dockspaceNames_[EditorLayoutType::Scene] = "SceneDockspace";
        dockspaceNames_[EditorLayoutType::ActorEditor] = "ActorEditorDockspace";

        layoutPanels_.emplace(EditorLayoutType::Scene, std::vector<GuiPanel*>{});
        layoutPanels_.emplace(EditorLayoutType::ActorEditor, std::vector<GuiPanel*>{});

        EnsureDockspaceForCurrentLayout_();
    }

    void GuiLayoutManager::Switch(EditorLayoutType newLayout)
    {
        if (!guiSystem_)
            return;

        if (currentLayout_ == newLayout)
        {
            if (switchRequested_)
            {
                switchRequested_ = false;
                pendingLayout_.reset();
            }
            return;
        }

        pendingLayout_ = newLayout;
        switchRequested_ = true;
    }

    void GuiLayoutManager::Render()
    {
        ProcessPendingSwitch_();
        EnsureDockspaceForCurrentLayout_();
    }

    void GuiLayoutManager::SaveCurrentLayout()
    {
        if (!guiSystem_ || !guiSystem_->IsInitialized())
            return;

        const std::string serialized = guiSystem_->SaveLayoutToMemory();
        if (!serialized.empty())
            layoutData_[currentLayout_] = serialized;
    }

    void GuiLayoutManager::LoadLayout(EditorLayoutType layout)
    {
        if (!guiSystem_ || !guiSystem_->IsInitialized())
            return;

        auto it = layoutData_.find(layout);
        if (it != layoutData_.end() && !it->second.empty())
        {
            guiSystem_->LoadLayoutFromMemory(it->second);
        }
        else
        {
            guiSystem_->RequestDefaultDockLayout();
        }
    }

    void GuiLayoutManager::SetPanelsForLayout(EditorLayoutType layout, const std::vector<GuiPanel*>& panels)
    {
        auto& layoutVector = layoutPanels_[layout];
        for (GuiPanel* panel : layoutVector)
        {
            if (!panel)
                continue;

            auto lookup = panelLayoutLookup_.find(panel);
            if (lookup != panelLayoutLookup_.end() && lookup->second == layout)
                panelLayoutLookup_.erase(lookup);
        }

        layoutVector.clear();

        layoutVector.reserve(panels.size());
        for (GuiPanel* panel : panels)
        {
            if (!panel)
                continue;

            if (std::find(layoutVector.begin(), layoutVector.end(), panel) != layoutVector.end())
                continue;

            layoutVector.push_back(panel);
            panelLayoutLookup_[panel] = layout;
        }

        ApplyPanelVisibility_();
    }

    void GuiLayoutManager::AddPanel(EditorLayoutType layout, GuiPanel& panel)
    {
        RemovePanel(panel);

        auto& layoutVector = layoutPanels_[layout];
        layoutVector.push_back(&panel);
        panelLayoutLookup_[&panel] = layout;

        if (currentLayout_ == layout)
            panel.SetVisible(true);
        else
            panel.SetVisible(false);
    }

    void GuiLayoutManager::RemovePanel(GuiPanel& panel)
    {
        auto lookup = panelLayoutLookup_.find(&panel);
        if (lookup == panelLayoutLookup_.end())
            return;

        RemovePanelFromLayout_(panel, lookup->second);
        panelLayoutLookup_.erase(lookup);
        panel.SetVisible(false);
    }

    void GuiLayoutManager::EnsureDockspaceForCurrentLayout_()
    {
        if (!guiSystem_)
            return;

        const std::string dockspaceName = dockspaceNames_[currentLayout_];
        const std::string dockspaceLabel = dockspaceName + "::DockSpace";

        if (!dockspaceDirty_)
            return;

        guiSystem_->SetDockspaceIdentifiers(kRootDockspaceWindow, dockspaceLabel);
        dockspaceDirty_ = false;
    }

    void GuiLayoutManager::ProcessPendingSwitch_()
    {
        if (!switchRequested_ || !pendingLayout_.has_value() || !guiSystem_)
            return;

        const EditorLayoutType newLayout = pendingLayout_.value();
        if (currentLayout_ == newLayout)
        {
            switchRequested_ = false;
            pendingLayout_.reset();
            return;
        }

        SaveCurrentLayout();

        currentLayout_ = newLayout;
        dockspaceDirty_ = true;
        EnsureDockspaceForCurrentLayout_();
        LoadLayout(newLayout);
        ApplyPanelVisibility_();

        switchRequested_ = false;
        pendingLayout_.reset();
    }

    void GuiLayoutManager::ApplyPanelVisibility_()
    {
        if (!guiManager_)
            return;

        std::unordered_set<GuiPanel*> visiblePanels;
        const auto& panels = layoutPanels_[currentLayout_];
        visiblePanels.insert(panels.begin(), panels.end());

        for (GuiPanel* panel : guiManager_->GetPanels())
        {
            if (!panel)
                continue;

            const bool shouldBeVisible = visiblePanels.find(panel) != visiblePanels.end();
            panel->SetVisible(shouldBeVisible);
        }
    }

    void GuiLayoutManager::RemovePanelFromLayout_(GuiPanel& panel, EditorLayoutType layout)
    {
        auto& layoutVector = layoutPanels_[layout];
        layoutVector.erase(std::remove(layoutVector.begin(), layoutVector.end(), &panel), layoutVector.end());
    }
}

