#pragma once
#include <string_view>
#include <vector>
#include "PropertyInfo.h"
#include "Gui/Controllers/BaseAssetEditorController.h"


namespace BixEngine::Gui::ActorInspector
{
    class PropertyInspector
    {
    public:
        PropertyInspector() = default;
        ~PropertyInspector() = default;

        // --- Reflection ---
        static bool DrawReflectedProperty(const Bix::Reflection::PropertyInfo& property, void* instance);
        static bool DrawClassProperties(const Bix::Reflection::ClassInfo& classInfo, void* instance, bool includeHeader = true, const char* headerLabel = "Properties", bool showEmptyMessage = true);

        // --- Exposed variables ---
        static void DrawExposedVariablesSection(BaseAssetEditorController::SharedState& state, std::string_view sectionLabel = "Variables", const char* emptyMessage = "No exposed variables.");

    private:

        static bool DrawSupportedProperty(const Bix::Reflection::PropertyInfo& property, void* instance, const std::string& label);

        static void GatherClassProperties(const Bix::Reflection::ClassInfo& cls, std::vector<const Bix::Reflection::PropertyInfo*>& out);
    };
}
