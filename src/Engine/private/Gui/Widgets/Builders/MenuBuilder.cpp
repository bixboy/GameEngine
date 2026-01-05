#include "Gui/Widgets/Builders/MenuBuilder.h"

#include "imgui.h"

namespace BixEngine::Gui::Widgets::Builder
{
    Menu::Menu(const char* label, bool enabled) noexcept
    {
        if (ImGui::BeginMenu(label, enabled))
        {
            Activate();
        }
    }

    Menu::~Menu()
    {
        if (IsActive())
            ImGui::EndMenu();
    }
}
