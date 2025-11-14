#include "Gui/Widgets/Builders/SectionBuilder.h"

namespace BixEngine::Gui::Widgets
{
    namespace Builder
    {
        Section::Section(const char* label, bool defaultOpen, ImGuiTreeNodeFlags flags)
            : section_(label, defaultOpen, flags)
        {
        }

        Section::~Section() = default;
    }
}
