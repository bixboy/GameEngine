#include "Gui/Widgets/Controls/DrawStringControl.h"
#include "Gui/Utils/MouseWrapping.h"
#include <vector>
#include "Gui/Utils/TextHelpers.h"


namespace BixEngine::Gui::Widgets
{
    namespace
    {
        struct InputTextCallback_UserData
        {
            std::string* Str;
        };

        int InputTextCallback(ImGuiInputTextCallbackData* data)
        {
            auto* user_data = static_cast<InputTextCallback_UserData*>(data->UserData);
            
            if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
            {
                std::string* str = user_data->Str;
                IM_ASSERT(data->Buf == str->c_str());
                
                str->resize(data->BufTextLen);
                
                data->Buf = const_cast<char*>(str->c_str());
            }
            return 0;
        }
    }

    bool DrawStringControl(const char* label, std::string& value, float columnWidth, ImGuiInputTextFlags flags)
    {
        ImGui::PushID(label);

        Utils::DrawPropertyLabel(label, columnWidth);
        
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

        InputTextCallback_UserData cb_user_data;
        cb_user_data.Str = &value;

        flags |= ImGuiInputTextFlags_CallbackResize;
        
        bool changed = ImGui::InputText("##StringVal", const_cast<char*>(value.c_str()), value.capacity() + 1, flags, InputTextCallback, &cb_user_data);

        ImGui::Columns(1);
        
        ImGui::PopID();
        return changed;
    }
}
