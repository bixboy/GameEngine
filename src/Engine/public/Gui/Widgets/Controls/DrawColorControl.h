#pragma once
#include "SDL3/SDL_pixels.h"

namespace BixEngine::Gui::Widgets
{
    bool DrawColorControl(const char* label, SDL_Color& color, float columnWidth = 120.0f);
}