#include "Engine/Gui/Core/GuiPanelRegistry.h"
#include "Engine/Gui/Core/GuiPanel.h"

namespace BixEngine::Gui
{
    GuiPanel& GuiPanelRegistry::AddPanel(String name, String title)
    {
        if (auto it = panels_.find(name); it != panels_.end())
        {
            if (it->second.panel)
                it->second.panel->SetTitle(std::move(title));
            
            return *it->second.panel;
        }

        PanelEntry entry{};
        entry.panel = std::make_unique<GuiPanel>(name, title);
        entry.panelType = std::type_index(typeid(GuiPanel));

        GuiPanel& panelRef = *entry.panel;
        RegisterPanelIndex_(panelRef, name);

        panels_.emplace(name, std::move(entry));

        if (OnPanelCreated)
            OnPanelCreated(panelRef);

        return panelRef;
    }

    void GuiPanelRegistry::RemovePanel(const String& name)
    {
        if (auto it = panels_.find(name); it != panels_.end())
        {
            auto& entry = it->second;
            if (entry.panel)
            {
                if (OnPanelRemoved)
                    OnPanelRemoved(*entry.panel);

                UnregisterPanelIndex_(*entry.panel);
            }

            panels_.erase(it);
        }
    }

    GuiPanel* GuiPanelRegistry::FindPanel(const String& name) noexcept
    {
        if (auto it = panels_.find(name); it != panels_.end())
            return it->second.panel.get();
        
        return nullptr;
    }

    const GuiPanel* GuiPanelRegistry::FindPanel(const String& name) const noexcept
    {
        if (auto it = panels_.find(name); it != panels_.end())
            return it->second.panel.get();
        
        return nullptr;
    }

    GuiPanelRegistry::PanelEntry* GuiPanelRegistry::FindPanelEntry(const String& name) noexcept
    {
        if (auto it = panels_.find(name); it != panels_.end())
            return &it->second;
        
        return nullptr;
    }

    GuiPanelRegistry::PanelEntry* GuiPanelRegistry::FindPanelEntry(GuiPanel& panel) noexcept
    {
        if (auto it = panelToName_.find(&panel); it != panelToName_.end())
            return FindPanelEntry(it->second);
        
        return nullptr;
    }

    const GuiPanelRegistry::PanelEntry* GuiPanelRegistry::FindPanelEntry(const String& name) const noexcept
    {
        if (auto it = panels_.find(name); it != panels_.end())
            return &it->second;
        
        return nullptr;
    }

    std::vector<GuiPanel*> GuiPanelRegistry::GetAllPanels()
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

    std::vector<const GuiPanel*> GuiPanelRegistry::GetAllPanels() const
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

    void GuiPanelRegistry::Clear()
    {
        panels_.clear();
        panelToName_.clear();
    }

    void GuiPanelRegistry::RegisterPanelIndex_(GuiPanel& panel, const String& name)
    {
        panelToName_[&panel] = name;
    }

    void GuiPanelRegistry::UnregisterPanelIndex_(GuiPanel& panel)
    {
        panelToName_.erase(&panel);
    }
}
