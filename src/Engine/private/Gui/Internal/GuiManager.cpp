#include "Gui/GuiManager.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

#include "Gui/Dialogs/ModalDialog.h"
#include "Gui/GuiPanelBase.h"
#include "Gui/Internal/GuiPanel.h"
#include "Gui/Internal/GuiSystem.h"
#include "imgui.h"

namespace BixEngine::Gui
{
    namespace
    {
        bool TitleHasStableSuffix(const String& title)
        {
            return title.View().find("###") != String::ViewType::npos;
        }

        String MakeStableTitle(String baseTitle, const String& identifier)
        {
            if (TitleHasStableSuffix(baseTitle))
                return baseTitle;

            baseTitle += "###";
            baseTitle += identifier;
            return baseTitle;
        }
    }

    GuiManager::GuiManager(GuiSystem& guiSystem) : guiSystem_(&guiSystem)
    {
        assetEditors_.SetGuiManager(this);

        registry_.OnPanelCreated = [this](GuiPanel& panel)
        {
            if (guiSystem_)
                guiSystem_->RegisterPanel(panel);

            panel.OnFocus = [this, &panel]()
            {
                history_.RecordVisit(panel.GetName());
            };

            if (OnPanelCreated)
                OnPanelCreated(panel);
        };

        registry_.OnPanelRemoved = [this](GuiPanel& panel)
        {
            OnPanelRemovedInternal_(panel);

            if (guiSystem_)
                guiSystem_->UnregisterPanel(panel);

            if (OnPanelRemoved)
                OnPanelRemoved(panel);
        };
    }

    GuiManager::~GuiManager()
    {
        registry_.Clear();
        childPanels_.Clear();
        history_.Clear();
    }

    GuiPanel& GuiManager::CreatePanel(String name, String title)
    {
        GuiPanel& panel = registry_.AddPanel(name, title);
        panel.SetTitle(MakeStableTitle(panel.GetTitle(), panel.GetName()));
        return panel;
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
        for (GuiPanel* panel : registry_.GetAllPanels())
        {
            if (!panel)
                continue;
            panel->Draw();
        }
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
            throw std::runtime_error("GuiManager::AttachController — panel introuvable");

        return AttachController(*entry->panel, std::move(controller));
    }

    GuiPanelController& GuiManager::AttachController(GuiPanel& panel, std::unique_ptr<GuiPanelController> controller)
    {
        auto* entry = registry_.FindPanelEntry(panel);
        if (!entry || !controller)
            throw std::runtime_error("GuiManager::AttachController — paramètres invalides");

        if (entry->controller)
        {
            childPanels_.RemoveChildren(*entry->controller, [this](const String& childName)
                                       { RemovePanel(childName); });
            entry->controller->DetachFromPanel();
            entry->controller->UnbindManager();
        }

        controller->BindManager(*this);
        controller->AttachToPanel(*entry->panel);

        GuiPanelController& ref = *controller;
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
                childPanels_.RemoveChildren(*entry->controller, [this](const String& childName)
                                            { RemovePanel(childName); });
                entry->controller->DetachFromPanel();
                entry->controller->UnbindManager();
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
                childPanels_.RemoveChildren(*entry->controller, [this](const String& childName)
                                            { RemovePanel(childName); });
                entry->controller->DetachFromPanel();
                entry->controller->UnbindManager();
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

    GuiPanel& GuiManager::OpenChildPanel(GuiPanelController& parent, const GuiPanelController::ChildPanelConfig& config)
    {
        String finalName = config.name;
        if (finalName.IsEmpty())
        {
            finalName = parent.GetPanel().GetName();
            finalName += "::Child";
            finalName += std::to_string(childCounter_++).c_str();
        }

        String finalTitle = config.title.IsEmpty() ? String{"Child Panel"} : config.title;
        GuiPanel& panel = CreatePanel(finalName, finalTitle);

        panel.SetWindowFlags(config.windowFlags);
        panel.SetClosable(true);
        panel.SetVisible(true);

        switch (config.kind)
        {
        case GuiPanelController::ChildPanelKind::FloatingWindow:
            panel.ResetDockingPreference();
            break;
        case GuiPanelController::ChildPanelKind::SecondaryDocked:
            panel.SetDockingPreference(config.dockRegion, config.dockCondition);
            break;
        case GuiPanelController::ChildPanelKind::PersistentPopup:
            panel.ResetDockingPreference();
            panel.AddWindowFlags(ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking);
            break;
        }

        if (config.requestFocus)
            panel.RequestFocus();

        childPanels_.RegisterChild(parent, panel.GetName(), config.closeWithParent);
        return panel;
    }

    void GuiManager::CloseChildPanels(GuiPanelController& parent)
    {
        childPanels_.RemoveChildren(parent, [this](const String& childName)
        { RemovePanel(childName); });
    }

    bool GuiManager::NavigateBack()
    {
        if (const String* target = history_.NavigateBack())
            return FocusPanel(*target);
        return false;
    }

    bool GuiManager::NavigateForward()
    {
        if (const String* target = history_.NavigateForward())
            return FocusPanel(*target);
        return false;
    }

    void GuiManager::NavigateHome()
    {
        if (const String* target = history_.NavigateHome())
            FocusPanel(*target);
    }

    bool GuiManager::FocusPanel(const String& name)
    {
        if (GuiPanel* panel = FindPanel(name))
        {
            panel->SetVisible(true);
            panel->RequestFocus();
            return true;
        }
        return false;
    }

    void GuiManager::RegisterWorkspace(WorkspaceRegistry::Workspace workspace)
    {
        workspaces_.RegisterWorkspace(std::move(workspace));
    }

    bool GuiManager::ActivateWorkspace(const String& name)
    {
        return workspaces_.ActivateWorkspace(name, *this);
    }

    const WorkspaceRegistry::Workspace* GuiManager::GetActiveWorkspace() const noexcept
    {
        return workspaces_.GetActiveWorkspace();
    }

    void GuiManager::RegisterLayoutManager(GuiLayoutManager& layoutManager) noexcept
    {
        layoutManager_ = &layoutManager;
        workspaces_.SetLayoutManager(&layoutManager);
    }

    void GuiManager::AttachDrawFunction_(GuiPanelRegistry::PanelEntry& entry)
    {
        if (!entry.panel || !entry.controller)
            return;

        GuiPanel* panel = entry.panel.get();
        GuiPanelController* controller = entry.controller.get();

        panel->SetDrawFunction([this, controller, panel]()
        {
            controller->DrawPanel();
            if (panel->IsFocused())
                history_.RecordVisit(panel->GetName());
        });
    }

    void GuiManager::OnPanelRemovedInternal_(GuiPanel& panel)
    {
        assetEditors_.UnregisterPanel(panel.GetName());
        childPanels_.UnregisterChildByName(panel.GetName());

        if (auto* entry = registry_.FindPanelEntry(panel))
        {
            if (entry->controller)
            {
                entry->controller->DetachFromPanel();
                entry->controller->UnbindManager();
                entry->controller.reset();
            }
        }
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
                identifier += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            else if (ch == ' ' || ch == '-' || ch == ':')
                identifier += '_';
        }

        if (identifier.IsEmpty())
            identifier = "panel";

        return identifier;
    }
}
