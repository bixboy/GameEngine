#include "Engine/Gui/GuiManager.h"

#include <utility>
#include <vector>

#include "Engine/Gui/GuiPanel.h"
#include "Engine/Gui/GuiSystem.h"

namespace BixEngine::Gui
{
    GuiManager::GuiManager(GuiSystem& guiSystem) : guiSystem_(&guiSystem) {}

    GuiManager::~GuiManager()
    {
        if (!guiSystem_)
            return;

        for (auto& [_, panel] : panels_)
        {
            if (panel)
                guiSystem_->UnregisterPanel(*panel);
        }
    }

    GuiPanel& GuiManager::CreatePanel(String name, String title)
    {
        if (auto it = panels_.find(name); it != panels_.end())
        {
            it->second->SetTitle(std::move(title));
            return *it->second;
        }

        String key = name;
        auto panel = std::make_unique<GuiPanel>(std::move(name), std::move(title));
        GuiPanel& panelRef = *panel;
        panels_.emplace(std::move(key), std::move(panel));

        if (guiSystem_)
            guiSystem_->RegisterPanel(panelRef);

        return panelRef;
    }

    void GuiManager::RemovePanel(const String& name)
    {
        if (auto it = panels_.find(name); it != panels_.end())
        {
            if (guiSystem_ && it->second)
                guiSystem_->UnregisterPanel(*it->second);

            panels_.erase(it);
        }
    }

    GuiPanel* GuiManager::FindPanel(const String& name) noexcept
    {
        if (auto it = panels_.find(name); it != panels_.end())
            return it->second.get();

        return nullptr;
    }

    const GuiPanel* GuiManager::FindPanel(const String& name) const noexcept
    {
        if (auto it = panels_.find(name); it != panels_.end())
            return it->second.get();

        return nullptr;
    }

    void GuiManager::SetPanelDockingArea(const String& name, DockSpaceRegion area, ImGuiCond condition)
    {
        if (GuiPanel* panel = FindPanel(name))
            SetPanelDockingArea(*panel, area, condition);
    }

    void GuiManager::SetPanelDockingArea(GuiPanel& panel, DockSpaceRegion area, ImGuiCond condition)
    {
        panel.SetDockingPreference(area, condition);

        if (guiSystem_)
            guiSystem_->EnqueueDockUpdate(panel);
    }

    void GuiManager::DrawAll()
    {
        for (auto& [_, panel] : panels_)
        {
            if (panel)
                panel->Draw();
        }
    }

    std::vector<GuiPanel*> GuiManager::GetPanels()
    {
        std::vector<GuiPanel*> result;
        result.reserve(panels_.size());

        for (auto& [_, panel] : panels_)
        {
            if (panel)
                result.push_back(panel.get());
        }

        return result;
    }

    std::vector<const GuiPanel*> GuiManager::GetPanels() const
    {
        std::vector<const GuiPanel*> result;
        result.reserve(panels_.size());

        for (const auto& [_, panel] : panels_)
        {
            if (panel)
                result.push_back(panel.get());
        }

        return result;
    }
}
