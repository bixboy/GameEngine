#pragma once
#include "Gui/Widgets/ImGuiScopeBase.h"
#include "Gui/Widgets/Layout/PanelSection.h"

namespace BixEngine::Gui::Widgets
{
     
    class Section : public ImGuiScopeBase
    {
    public:
        Section(const char* label, bool defaultOpen = true, ImGuiTreeNodeFlags flags = 0) noexcept;
        ~Section();

        Section(const Section&) = delete;
        Section& operator=(const Section&) = delete;
        
        Section(Section&&) = delete;
        Section& operator=(Section&&) = delete;
        
        operator bool() const { return IsActive(); }
    };
}
