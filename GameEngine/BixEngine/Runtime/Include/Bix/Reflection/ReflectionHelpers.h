#pragma once

#include "Bix/Reflection/Registry.h"

#include <string_view>
#include <vector>

namespace Bix::Reflection
{
    /**
     * @brief Finds a class by its simple name.
     */
    inline ClassInfo* FindClass(std::string_view name)
    {
        return Registry::Get().Find(name);
    }

    /**
     * @brief Finds a class by its fully qualified name.
     */
    inline ClassInfo* FindClassByQualifiedName(std::string_view name)
    {
        return Registry::Get().FindByQualifiedName(name);
    }

    /**
     * @brief Returns every registered class metadata entry.
     */
    inline std::vector<ClassInfo*> GetAllClasses()
    {
        return Registry::Get().GetClasses();
    }
}
