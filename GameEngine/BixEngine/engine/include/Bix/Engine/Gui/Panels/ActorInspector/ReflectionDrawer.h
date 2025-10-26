#pragma once

#include "Bix/Reflection/BixReflection.h"

#include <string>
#include <string_view>
#include <vector>

namespace BixEngine::Gui::ActorInspector
{
    namespace PropertyUtils
    {
        [[nodiscard]] bool MatchesType(const std::string& typeName, std::string_view expectedSuffix);
        [[nodiscard]] std::string MakeDisplayName(const std::string& rawName);
    }

    bool DrawSupportedProperty(const ::Bix::Reflection::PropertyInfo& property, void* instance, const std::string& label);
    void DrawUnsupportedProperty(const ::Bix::Reflection::PropertyInfo& property, const std::string& label);
    bool DrawReflectedProperty(const ::Bix::Reflection::PropertyInfo& property, void* instance);

    void GatherClassProperties(const ::Bix::Reflection::ClassInfo& classInfo, std::vector<const ::Bix::Reflection::PropertyInfo*>& outProperties);
    bool IsSubclassOf(const ::Bix::Reflection::ClassInfo& type, const ::Bix::Reflection::ClassInfo& base);

    bool DrawClassProperties(const ::Bix::Reflection::ClassInfo& classInfo,
                             void* instance,
                             bool includeHeader,
                             const char* headerLabel,
                             bool showEmptyMessage);
}

