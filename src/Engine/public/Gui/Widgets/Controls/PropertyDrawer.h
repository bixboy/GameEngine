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

        static bool DrawAssetPicker(const std::string& currentPath, std::string& outNewPath, const std::vector<std::string>& extensions);
    };

    template <typename T>
    bool PropertyDrawer::DrawResourcePicker(const Reflection::PropertyInfo& property, void* instance, const std::vector<std::string>& extensions)
    {
        std::shared_ptr<T>& resourcePtr = property.Get<std::shared_ptr<T>>(instance);

        std::string currentPath = resourcePtr ? resourcePtr->GetPath().Std() : "";
        std::string newPath;

        if (DrawAssetPicker(currentPath, newPath, extensions))
        {
            resourcePtr = Resources::ResourceManager::Get().Get<T>(newPath);
            return true;
        }
        return false;
    }
}
