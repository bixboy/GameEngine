#pragma once
#include "Gui/Widgets/Internal/ImGuiScopeBase.h"
#include "imgui.h"

namespace BixEngine::Gui::Widgets::Builder
{
    /**
     * \brief RAII pour ImGui::BeginMenu/EndMenu avec gestion de l'état ouvert/fermé.
     */
    class Menu : private Internal::ImGuiScopeBase
    {
    public:
        Menu(const char* label, bool enabled = true) noexcept;
        ~Menu();

        Menu(const Menu&) = delete;
        Menu& operator=(const Menu&) = delete;
        Menu(Menu&&) = delete;
        Menu& operator=(Menu&&) = delete;

        [[nodiscard]] bool IsOpen() const noexcept { return open_; }

    private:
        bool open_{false};
    };
}
