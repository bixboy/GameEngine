#pragma once

#include "Core/Containers/String.h"

namespace BixEngine::Gui
{
    struct ContentBrowserState;
    struct PopupRequestState;

    void RenderEntries(ContentBrowserState& state, String& selectedEntry, PopupRequestState& requestPopups, const String& searchQuery);
}

