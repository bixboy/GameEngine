#pragma once

#include <cstdint>

namespace BixEngine::Gui
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
