#pragma once
#include "Core/PropertyInfo.h"
#include <vector>
#include <string>

#include "Gui/Core/DefaultEngineGui.h"
#include "Gui/Utils/ContentBrowserUtils.h"
#include "Ressources/Core/ResourceManager.h"

namespace BixEngine::Gui
{
    
    class PropertyDrawer
    {
    public:
        static bool DrawPropertyRow(const Reflection::PropertyInfo& prop, void* instance);

    private:
        static bool DispatchWidget(const Reflection::PropertyInfo& prop, void* instance);
        
        // --- Widgets Spécifiques ---
        
        static bool DrawBool(const Reflection::PropertyInfo& p, void* i);
        static bool DrawInt(const Reflection::PropertyInfo& p, void* i);
        static bool DrawFloat(const Reflection::PropertyInfo& p, void* i);
        static bool DrawDouble(const Reflection::PropertyInfo& p, void* i);
        static bool DrawVector2(const Reflection::PropertyInfo& p, void* i);
        static bool DrawVector3(const Reflection::PropertyInfo& p, void* i);
        static bool DrawColor(const Reflection::PropertyInfo& p, void* i);
        static bool DrawString(const Reflection::PropertyInfo& p, void* i);
        
        // --- Widgets Complexes ---
        
        static bool DrawTexture(const Reflection::PropertyInfo& p, void* i);
        static bool DrawTSubclassOf(const Reflection::PropertyInfo& p, void* i);
        
        // --- Containers ---
        
        static bool DrawStringArray(const Reflection::PropertyInfo& p, void* i);
        static bool DrawTSubclassOfArray(const Reflection::PropertyInfo& p, void* i);

        template <typename T>
        static bool DrawResourcePicker(const Reflection::PropertyInfo& property, void* instance, const std::vector<std::string>& extensions);
    };

    template <typename T>
    bool PropertyDrawer::DrawResourcePicker(const Reflection::PropertyInfo& property, void* instance, const std::vector<std::string>& extensions)
    {
        std::shared_ptr<T>& resourcePtr = property.Get<std::shared_ptr<T>>(instance);

            std::string currentPath = resourcePtr ? resourcePtr->GetPath().Std() : "";
            std::string previewStr = resourcePtr ? std::filesystem::path(currentPath).filename().string() : "None";

            float availableWidth = ImGui::GetContentRegionAvail().x;
            ImGui::SetNextItemWidth(availableWidth);

            bool changed = false;

            if (ImGui::BeginCombo("##ResourceCombo", previewStr.c_str()))
            {
                if (ImGui::Selectable("None", !resourcePtr))
                {
                    resourcePtr.reset();
                    changed = true;
                }

                std::vector<std::filesystem::path> files = Utils::FileUtils::ScanDirectory(ContentBrowserUtils::GetContentRoot(), extensions);
                
                for (const auto& file : files)
                {
                    std::string display = file.filename().string();
                    bool selected = (currentPath == file.generic_string());
                    
                    if (ImGui::Selectable(display.c_str(), selected))
                    {
                        resourcePtr = Resources::ResourceManager::Get().Get<T>(file.generic_string());
                        changed = true;
                    }
                    
                    if (selected)
                        ImGui::SetItemDefaultFocus();
                }
                
                ImGui::EndCombo();
            }
            
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                {
                    const char* path = static_cast<const char*>(payload->Data);
                    auto newRes = Resources::ResourceManager::Get().Get<T>(path);
                    
                    if (newRes)
                    {
                        resourcePtr = newRes;
                        changed = true;
                    }
                }
                
                ImGui::EndDragDropTarget();
            }

            return changed;
    }
}
