#include "Gui/Widgets/Controls/DrawBadge.h"
#include "Gui/Widgets/Styling/ThemeHelpers.h"
#include "imgui.h"
#include "Gui/Utils/TextHelpers.h"


namespace BixEngine::Gui::Widgets
{
    ImVec2 DrawBadge(const char* label, const ImVec4& backgroundColor, const ImVec4& textColor)
    {
        if (!label || label[0] == '\0')
            return {0.0f, 0.0f};

        const ImVec2 padding{6.0f, 3.0f};
        
        const ImVec2 size = Utils::ComputeBadgeSize(label, padding);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        
        const float rounding = size.y * 0.5f;

        const ImVec2 maxRect(pos.x + size.x, pos.y + size.y);
        
        drawList->AddRectFilled(pos, maxRect, ThemeHelpers::ToColor32(backgroundColor), rounding);
        
        ImVec2 textSize = ImGui::CalcTextSize(label);
        ImVec2 textPos;
        textPos.x = pos.x + (size.x - textSize.x) * 0.5f;
        textPos.y = pos.y + (size.y - textSize.y) * 0.5f;

        drawList->AddText(textPos, ThemeHelpers::ToColor32(textColor), label);

        ImGui::Dummy(size);
        
        return size;
    }
}
