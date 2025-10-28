#include "Engine/Gui/Core/GuiManager.h"
#include "Engine/Gui/Core/GuiPanel.h"
#include "Engine/Gui/Core/GuiSystem.h"
#include "Engine/Gui/Utils/GuiPanelController.h"

namespace BixEngine::Gui
{
    GuiManager::GuiManager(GuiSystem& guiSystem) : guiSystem_(&guiSystem)
    {
        registry_.OnPanelCreated = [this](GuiPanel& panel)
        {
            if (guiSystem_)
                guiSystem_->RegisterPanel(panel);
            
            if (OnPanelCreated)
                OnPanelCreated(panel);
        };

        registry_.OnPanelRemoved = [this](GuiPanel& panel)
        {
            if (guiSystem_)
                guiSystem_->UnregisterPanel(panel);
            
            if (OnPanelRemoved)
                OnPanelRemoved(panel);
        };
    }

    GuiManager::~GuiManager()
    {
        registry_.Clear();
    }

    GuiPanel& GuiManager::CreatePanel(String name, String title)
    {
        return registry_.AddPanel(std::move(name), std::move(title));
    }

    void GuiManager::RemovePanel(const String& name)
    {
        registry_.RemovePanel(name);
    }

    GuiPanel* GuiManager::FindPanel(const String& name) noexcept
    {
        return registry_.FindPanel(name);
    }

    const GuiPanel* GuiManager::FindPanel(const String& name) const noexcept
    {
        return registry_.FindPanel(name);
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
        for (auto* panel : registry_.GetAllPanels())
            panel->Draw();
    }

    std::vector<GuiPanel*> GuiManager::GetPanels()
    {
        return registry_.GetAllPanels();
    }

    std::vector<const GuiPanel*> GuiManager::GetPanels() const
    {
        return registry_.GetAllPanels();
    }

    GuiPanelController& GuiManager::AttachController(const String& name, std::unique_ptr<GuiPanelController> controller)
    {
        auto* entry = registry_.FindPanelEntry(name);
        if (!entry || !entry->panel)
            throw std::runtime_error("Panel not found in registry.");

        return AttachController(*entry->panel, std::move(controller));
    }

    GuiPanelController& GuiManager::AttachController(GuiPanel& panel, std::unique_ptr<GuiPanelController> controller)
    {
        auto* entry = registry_.FindPanelEntry(panel);
        if (!entry || !controller)
            throw std::runtime_error("Invalid controller attachment.");

        if (entry->controller)
            entry->controller->DetachFromPanel();

        GuiPanelController& ref = *controller;
        controller->AttachToPanel(*entry->panel);
        entry->controller = std::move(controller);

        AttachDrawFunction_(*entry);
        return ref;
    }

    void GuiManager::DetachController(const String& name)
    {
        if (auto* entry = registry_.FindPanelEntry(name))
        {
            if (entry->controller)
            {
                entry->controller->DetachFromPanel();
                entry->controller.reset();
            }
            entry->panel->SetDrawFunction(nullptr);
        }
    }

    void GuiManager::DetachController(GuiPanel& panel)
    {
        if (auto* entry = registry_.FindPanelEntry(panel))
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
        if (auto* entry = registry_.FindPanelEntry(name))
            return entry->controller.get();
        
        return nullptr;
    }

    const GuiPanelController* GuiManager::GetController(const String& name) const noexcept
    {
        if (auto* entry = registry_.FindPanelEntry(name))
            return entry->controller.get();
        
        return nullptr;
    }

    void GuiManager::AttachDrawFunction_(GuiPanelRegistry::PanelEntry& entry)
    {
        if (!entry.panel)
            return;

        if (entry.controller)
        {
            GuiPanelController* ctrl = entry.controller.get();
            entry.panel->SetDrawFunction([ctrl]() { ctrl->DrawPanel(); });
        }
        else
        {
            entry.panel->SetDrawFunction(nullptr);
        }
    }
}
