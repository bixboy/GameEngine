#include "Gui/Widgets/Builders/MenuBuilder.h"

#include "imgui.h"

namespace BixEngine::Gui::Widgets::Builder
{
    Menu::Menu(const char* label, bool enabled) noexcept
    {
        open_ = ImGui::BeginMenu(label, enabled);
        if (open_)
            Activate();
        else
            Deactivate();
    }

    Menu::~Menu()
    {
        if (IsActive())
            ImGui::EndMenu();
    }
}
