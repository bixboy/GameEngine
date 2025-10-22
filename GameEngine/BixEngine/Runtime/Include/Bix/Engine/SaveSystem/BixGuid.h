#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "Bix/Core/String.h"

namespace BixEngine::Engine::SaveSystem
{
    /**
     * Represents a globally unique identifier used by the save system to
     * uniquely identify every reflected object.
     */
    struct BixGuid
    {
        std::array<std::uint8_t, 16> data{};

        [[nodiscard]] bool IsValid() const noexcept;
        [[nodiscard]] String ToString() const;

        static BixGuid NewGuid();
        static BixGuid FromString(std::string_view text);

        friend bool operator==(const BixGuid& lhs, const BixGuid& rhs) noexcept
        {
            return lhs.data == rhs.data;
        }

        friend bool operator!=(const BixGuid& lhs, const BixGuid& rhs) noexcept
        {
            return !(lhs == rhs);
        }
    };

    struct BixGuidHasher
    {
        std::size_t operator()(const BixGuid& guid) const noexcept;
    };
} // namespace BixEngine::Engine::SaveSystem
