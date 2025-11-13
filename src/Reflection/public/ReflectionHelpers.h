#pragma once
#include "Registry.h"
#include <string_view>
#include <vector>


namespace Bix::Reflection
{
    inline ClassInfo* FindClass(std::string_view name)
    {
        return Registry::Get().Find(name);
    }

    inline ClassInfo* FindClassByQualifiedName(std::string_view name)
    {
        return Registry::Get().FindByQualifiedName(name);
    }

    inline std::vector<ClassInfo*> GetAllClasses()
    {
        return Registry::Get().GetClasses();
    }
}
