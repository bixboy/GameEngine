#pragma once

#include <cstddef>
#include <functional>
#include <string>

namespace Bix::Reflection
{
    struct ClassInfo;

    /**
     * @brief Holds metadata describing a single reflected property.
     */
    struct PropertyInfo
    {
        using Getter = std::function<void*(void*)>;
        using ConstGetter = std::function<const void*(const void*)>;

        std::string Name;
        std::string TypeName;
        std::size_t Offset = 0;
        bool HasOffset = false;
        std::size_t Size = 0;
        const ClassInfo* Owner = nullptr;
        Getter Access;
        ConstGetter ConstAccess;

        /**
         * @brief Checks that both accessors are valid.
         */
        [[nodiscard]] bool IsValid() const noexcept;

        /**
         * @brief Retrieves the property pointer from a mutable instance.
         */
        void* GetRaw(void* instance) const;

        /**
         * @brief Retrieves the property pointer from an immutable instance.
         */
        const void* GetRaw(const void* instance) const;

        template<typename T>
        /**
         * @brief Convenience helper that returns a typed reference for mutable access.
         */
        T& Get(void* instance) const
        {
            return *static_cast<T*>(GetRaw(instance));
        }

        template<typename T>
        /**
         * @brief Convenience helper that returns a typed reference for immutable access.
         */
        const T& Get(const void* instance) const
        {
            return *static_cast<const T*>(GetRaw(instance));
        }
    };
}
