#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace BixTool {

    inline constexpr std::string_view kBClassMacro    = "BCLASS()";
    inline constexpr std::string_view kBPropertyMacro = "BPROPERTY()";

    struct Property {
        std::string Type;
        std::string Name;
    };

    struct ParsedClass {
        std::string Name;
        std::vector<std::string> Namespaces;
        std::string BaseType;
        std::vector<Property> Properties;
        bool HasGeneratedBody = false;
    };

    struct HeaderParseResult {
        bool ContainsReflection = false;
        std::vector<ParsedClass> Classes;
    };

    enum class ScopeType { Namespace, Other };

    struct ScopeEntry {
        ScopeType Type = ScopeType::Other;
        std::size_t NamespaceCount = 0;
    };

}
