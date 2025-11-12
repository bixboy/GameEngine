#include "Gui/GuiManager.h"
#include "Gui/Internal/GuiPanel.h"
#include "Gui/Internal/GuiPanelRegistry.h"
#include "Gui/Internal/GuiSystem.h"
#include "Gui/Controllers/GuiPanelController.h"
#include "Gui/GuiPanelBase.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <unordered_map>

#include "Gui/GuiDocking.h"
#include "Gui/Dialogs/ModalDialog.h"

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

    void GuiManager::RemovePanels(std::span<GuiPanel*> panels)
    {
        for (GuiPanel* panel : panels)
        {
            if (!panel)
                continue;
            RemovePanel(panel->GetName());
        }
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

    std::unordered_map<std::string, GuiManager::RegisteredPanel>& GuiManager::StaticPanelRegistry_()
    {
        static std::unordered_map<std::string, RegisteredPanel> registry;
        return registry;
    }

    String GuiManager::SanitizeIdentifier_(const String& name)
    {
        String identifier;
        identifier.reserve(name.size());

        for (char ch : name.View())
        {
            if (std::isalnum(static_cast<unsigned char>(ch)))
            {
                identifier += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            }
            else if (ch == ' ' || ch == '-' || ch == ':')
            {
                identifier += '_';
            }
        }

        if (identifier.IsEmpty())
            identifier = "panel";

        return identifier;
    }

    void GuiManager::UnregisterPanel(const String& displayName)
    {
        StaticPanelRegistry_().erase(displayName.Std());
    }

    GuiPanelBase* GuiManager::CreatePanelByName(const String& displayName)
    {
        auto& staticRegistry = StaticPanelRegistry_();
        const auto it = staticRegistry.find(displayName.Std());
        if (it == staticRegistry.end())
            return nullptr;

        RegisteredPanel& entry = it->second;
        if (!entry.factory)
            return nullptr;

        if (GuiPanelBase* existingController = GetControllerAs<GuiPanelBase>(entry.identifier))
            return existingController;

        auto controller = entry.factory();
        if (!controller)
            return nullptr;

        GuiPanel& panel = CreatePanel(entry.identifier, entry.displayName);
        GuiPanelBase* controllerPtr = controller.get();
        AttachController(panel, std::move(controller));
        return controllerPtr;
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
