#pragma once
#include <string>
#include <string_view>
#include <vector>
#include "Core/PropertyInfo.h"


namespace BixEngine::Reflection
{
    struct ClassInfo
    {
        using Constructor = void* (*)(void* context);

        std::string Name;
        std::string QualifiedName;
        std::size_t Size = 0;
        ClassInfo* SuperClass = nullptr;
        std::vector<PropertyInfo> Properties;
        bool IsAbstract = false;
        Constructor ConstructorFn = nullptr;

        [[nodiscard]] const PropertyInfo* FindProperty(std::string_view name) const;

        PropertyInfo* FindProperty(std::string_view name);

        [[nodiscard]] bool CanConstruct() const noexcept;

        void* Construct(void* context = nullptr) const;

        template <typename T>
        T* ConstructTyped(void* context = nullptr) const
        {
            return static_cast<T*>(Construct(context));
        }
    };
}
