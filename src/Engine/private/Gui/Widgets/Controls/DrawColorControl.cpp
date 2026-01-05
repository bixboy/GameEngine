#include "Gui/Widgets/Controls/DrawColorControl.h"
#include "imgui.h"
#include <cmath> 
#include "Gui/Utils/TextHelpers.h"


namespace BixEngine::Gui::Widgets
{
    bool DrawColorControl(const char* label, SDL_Color& color, float columnWidth)
    {
        ImGui::PushID(label);

        Utils::DrawPropertyLabel(label, columnWidth);

        float col[4] = {
            static_cast<float>(color.r) / 255.0f,
            static_cast<float>(color.g) / 255.0f,
            static_cast<float>(color.b) / 255.0f,
            static_cast<float>(color.a) / 255.0f
        };
        
        bool changed = ImGui::ColorEdit4("##ColorVal", col);

        if (changed)
        {
            color.r = static_cast<Uint8>(std::lround(col[0] * 255.0f));
            color.g = static_cast<Uint8>(std::lround(col[1] * 255.0f));
            color.b = static_cast<Uint8>(std::lround(col[2] * 255.0f));
            color.a = static_cast<Uint8>(std::lround(col[3] * 255.0f));
        }

        ImGui::Columns(1);
        ImGui::PopID();
        
        return changed;
    }
}
