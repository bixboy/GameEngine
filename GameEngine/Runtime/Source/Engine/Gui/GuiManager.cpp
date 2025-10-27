#include "Engine/Gui/GuiManager.h"

#include <utility>
#include <vector>

#include "Engine/Gui/GuiPanel.h"
#include "Engine/Gui/GuiPanelController.h"
#include "Engine/Gui/GuiSystem.h"

#include <stdexcept>

namespace BixEngine::Gui
{
    GuiManager::GuiManager(GuiSystem& guiSystem) : guiSystem_(&guiSystem) {}

    GuiManager::~GuiManager()
    {
        if (!guiSystem_)
            return;

        for (auto& [_, entry] : panels_)
        {
            if (entry.controller)
                entry.controller->DetachFromPanel();

            if (entry.panel)
                guiSystem_->UnregisterPanel(*entry.panel);
        }
    }

    GuiPanel& GuiManager::CreatePanel(String name, String title)
    {
        if (auto it = panels_.find(name); it != panels_.end())
        {
            if (it->second.panel)
                it->second.panel->SetTitle(std::move(title));
            return *it->second.panel;
        }

        String key = name;
        PanelEntry entry{};
        entry.panel = std::make_unique<GuiPanel>(std::move(name), std::move(title));

        GuiPanel& panelRef = *entry.panel;
        panels_.emplace(std::move(key), std::move(entry));

        if (guiSystem_)
            guiSystem_->RegisterPanel(panelRef);

        return panelRef;
    }

    void GuiManager::RemovePanel(const String& name)
    {
        if (auto it = panels_.find(name); it != panels_.end())
        {
            if (auto& entry = it->second; entry.panel)
            {
                if (entry.controller)
                    entry.controller->DetachFromPanel();

                if (guiSystem_)
                    guiSystem_->UnregisterPanel(*entry.panel);
            }

            panels_.erase(it);
        }
    }

    GuiPanel* GuiManager::FindPanel(const String& name) noexcept
    {
        if (auto it = panels_.find(name); it != panels_.end())
            return it->second.panel.get();

        return nullptr;
    }

    const GuiPanel* GuiManager::FindPanel(const String& name) const noexcept
    {
        if (auto it = panels_.find(name); it != panels_.end())
            return it->second.panel.get();

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
        for (auto& [_, entry] : panels_)
        {
            if (entry.panel)
                entry.panel->Draw();
        }
    }

    std::vector<GuiPanel*> GuiManager::GetPanels()
    {
        std::vector<GuiPanel*> result;
        result.reserve(panels_.size());

        for (auto& [_, entry] : panels_)
        {
            if (entry.panel)
                result.push_back(entry.panel.get());
        }

        return result;
    }

    std::vector<const GuiPanel*> GuiManager::GetPanels() const
    {
        std::vector<const GuiPanel*> result;
        result.reserve(panels_.size());

        for (const auto& [_, entry] : panels_)
        {
            if (entry.panel)
                result.push_back(entry.panel.get());
        }

        return result;
    }

    GuiPanelController& GuiManager::AttachController(const String& name, std::unique_ptr<GuiPanelController> controller)
    {
        PanelEntry* entry = FindPanelEntry(name);
        if (!entry || !entry->panel)
            throw std::runtime_error("Panel not managed by GuiManager.");

        return AttachController(*entry->panel, std::move(controller));
    }

    GuiPanelController& GuiManager::AttachController(GuiPanel& panel, std::unique_ptr<GuiPanelController> controller)
    {
        PanelEntry* entry = FindPanelEntry(panel);
        if (!entry || !entry->panel || !controller)
            throw std::runtime_error("Invalid panel controller attachment request.");

        if (entry->controller)
            entry->controller->DetachFromPanel();

        GuiPanelController& controllerRef = *controller;
        controller->AttachToPanel(*entry->panel);
        entry->controller = std::move(controller);

        AttachDrawFunction_(*entry);
        return controllerRef;
    }

    void GuiManager::DetachController(const String& name)
    {
        if (PanelEntry* entry = FindPanelEntry(name))
        {
            if (entry->controller)
            {
                entry->controller->DetachFromPanel();
                entry->controller.reset();
            }

            if (entry->panel)
                entry->panel->SetDrawFunction(nullptr);
        }
    }

    void GuiManager::DetachController(GuiPanel& panel)
    {
        if (PanelEntry* entry = FindPanelEntry(panel))
        {
            if (entry->controller)
            {
                entry->controller->DetachFromPanel();
                entry->controller.reset();
            }

            entry->panel->SetDrawFunction(nullptr);
        }
    }

    GuiPanelController* GuiManager::GetController(const String& name) noexcept
    {
        if (PanelEntry* entry = FindPanelEntry(name))
            return entry->controller.get();
        return nullptr;
    }

    const GuiPanelController* GuiManager::GetController(const String& name) const noexcept
    {
        if (const PanelEntry* entry = FindPanelEntry(name))
            return entry->controller.get();
        return nullptr;
    }

    GuiManager::PanelEntry* GuiManager::FindPanelEntry(const String& name) noexcept
    {
        if (auto it = panels_.find(name); it != panels_.end())
            return &it->second;
        return nullptr;
    }

    GuiManager::PanelEntry* GuiManager::FindPanelEntry(GuiPanel& panel) noexcept
    {
        for (auto& [_, entry] : panels_)
        {
            if (entry.panel.get() == &panel)
                return &entry;
        }
        return nullptr;
    }

    const GuiManager::PanelEntry* GuiManager::FindPanelEntry(const String& name) const noexcept
    {
        if (auto it = panels_.find(name); it != panels_.end())
            return &it->second;
        return nullptr;
    }

    void GuiManager::AttachDrawFunction_(PanelEntry& entry)
    {
        if (!entry.panel)
            return;

        if (entry.controller)
        {
            GuiPanelController* controller = entry.controller.get();
            entry.panel->SetDrawFunction([controller]()
            {
                controller->DrawPanel();
            });
        }
        else
        {
            entry.panel->SetDrawFunction(nullptr);
        }
    }
}
