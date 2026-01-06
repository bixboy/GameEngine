#include "Gui/Widgets/Controls/DrawBoolControl.h"
#include "imgui.h"
#include "Gui/Utils/TextHelpers.h"

namespace BixEngine::Gui::Widgets
{
    bool DrawBoolControl(const char* label, bool& value, float columnWidth)
    {
        ImGui::PushID(label);

        bool changed = ImGui::Checkbox("##BoolVal", &value);

        // ImGui::Columns(1); // Removed to fix table layout
        ImGui::PopID();
        
        return changed;
    }
}
