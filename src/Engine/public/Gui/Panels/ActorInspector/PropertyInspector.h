#pragma once
#include <string_view>
#include "ClassInfo.h"
#include "Gui/Controllers/BaseAssetEditorController.h"


namespace BixEngine::Gui::ActorInspector
{
    
    bool DrawReflectedProperty(const Bix::Reflection::PropertyInfo& property, void* instance);

    bool DrawClassProperties(const Bix::Reflection::ClassInfo& classInfo, void* instance, bool includeHeader = true, const char* headerLabel = "Properties", bool showEmptyMessage = true);
    
    void DrawExposedVariablesSection(BaseAssetEditorController::SharedState& state, std::string_view sectionLabel = "Variables", const char* emptyMessage = "No exposed variables.");
}
