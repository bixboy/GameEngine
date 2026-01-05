#pragma once
#include "Core/PropertyInfo.h"


namespace BixEngine::Gui::ActorInspector
{
    class PropertyInspector
    {
    public:
        PropertyInspector() = default;
        ~PropertyInspector() = default;
        
        static bool DrawClassProperties(const Bix::Reflection::ClassInfo& classInfo, void* instance,
            bool includeHeader = true, const char* headerLabel = "Properties", bool showEmptyMessage = true);
    };
}
