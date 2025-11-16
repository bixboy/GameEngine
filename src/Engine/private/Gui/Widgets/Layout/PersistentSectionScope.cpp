#include "Gui/Widgets/Layout/PersistentSectionScope.h"

#include "Gui/Utils/GuiHelpers.h"

namespace BixEngine::Gui::Widgets
{
    PersistentSectionScope::PersistentSectionScope(const char* label, const std::string& contextId, bool defaultOpen,
                                                   ImGuiTreeNodeFlags flags)
        : isOpen_(Gui::Utils::BeginPersistentSection(label, contextId, defaultOpen, flags))
    {
    }

    PersistentSectionScope::~PersistentSectionScope()
    {
        if (isOpen_)
            Gui::Utils::EndPersistentSection();
    }
}
