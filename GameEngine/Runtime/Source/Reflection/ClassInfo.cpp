#include "Bix/Reflection/ClassInfo.h"

#include "Bix/Reflection/PropertyInfo.h"

namespace Bix::Reflection
{
    const PropertyInfo* ClassInfo::FindProperty(std::string_view name) const
    {
        for (const PropertyInfo& property : Properties)
        {
            if (property.Name == name)
            {
                return &property;
            }
        }

        return SuperClass ? SuperClass->FindProperty(name) : nullptr;
    }

    PropertyInfo* ClassInfo::FindProperty(std::string_view name)
    {
        for (PropertyInfo& property : Properties)
        {
            if (property.Name == name)
            {
                return &property;
            }
        }

        return SuperClass ? SuperClass->FindProperty(name) : nullptr;
    }

    bool ClassInfo::CanConstruct() const noexcept
    {
        return ConstructorFn != nullptr;
    }

    void* ClassInfo::Construct(void* context) const
    {
        return ConstructorFn ? ConstructorFn(context) : nullptr;
    }
}
