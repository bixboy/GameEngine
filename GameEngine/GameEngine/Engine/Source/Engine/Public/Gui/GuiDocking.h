#pragma once

#include <cstdint>

namespace Engine::Gui
{
    enum class DockSpaceRegion : std::uint8_t
    {
        Center = 0,
        Left,
        Right,
        Bottom,
        Top,
        Count
    };
}
