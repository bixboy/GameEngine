#include "Gui/Widgets/Builders/MenuBuilder.h"

namespace BixEngine::Gui::Widgets::Builder
{
    Menu::Menu(const char* label, bool enabled) noexcept
    {
        open_ = ImGui::BeginMenu(label, enabled);
    }

    Menu::~Menu()
    {
        if (open_)
            ImGui::EndMenu();
    }
}
