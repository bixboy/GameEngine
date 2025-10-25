#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace Bix::Reflection
{
    struct PropertyInfo;

    /**
     * @brief Holds metadata about a reflected class.
     */
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

        /**
         * @brief Searches for a property by name.
         */
        [[nodiscard]] const PropertyInfo* FindProperty(std::string_view name) const;

        /**
         * @brief Searches for a property by name with mutable access.
         */
        PropertyInfo* FindProperty(std::string_view name);

        /**
         * @brief Indicates whether the class has a registered constructor callback.
         */
        [[nodiscard]] bool CanConstruct() const noexcept;

        /**
         * @brief Invokes the registered constructor callback if available.
         */
        void* Construct(void* context = nullptr) const;

        template<typename T>
        /**
         * @brief Constructs an instance and casts it to the requested type.
         */
        T* ConstructTyped(void* context = nullptr) const
        {
            return static_cast<T*>(Construct(context));
        }
    };
}

#