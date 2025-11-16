#include "Gui/Widgets/Controls/DrawBadge.h"

#include "Gui/Widgets/Controls/TextHelpers.h"
#include "Gui/Widgets/Styling/ThemeHelpers.h"
#include "imgui.h"

namespace BixEngine::Gui::Widgets
{
    ImVec2 DrawBadge(const char* label, const ImVec4& backgroundColor, const ImVec4& textColor)
    {
        if (!label || label[0] == '\0')
            return {0.0f, 0.0f};

        const ImVec2 padding{6.0f, 3.0f};
        const ImVec2 size = Controls::TextHelpers::ComputeBadgeSize(label, padding);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImVec2 position = ImGui::GetCursorScreenPos();
        const float rounding = size.y * 0.45f;

        const ImVec2 max(position.x + size.x, position.y + size.y);
        drawList->AddRectFilled(position, max, ThemeHelpers::ToColor32(backgroundColor), rounding);
        drawList->AddText(ImVec2(position.x + padding.x, position.y + padding.y),
                          ThemeHelpers::ToColor32(textColor),
                          label);

        ImGui::Dummy(size);
        return size;
    }
}
