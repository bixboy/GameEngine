#pragma once
#include "Containers/String.h"


namespace BixEngine::Gui
{
    struct ContentBrowserState;
    struct PopupRequestState;
    struct ContentEntry;

    void RenderPopups(ContentBrowserState& state, String& selectedEntry, PopupRequestState& requestPopups);
}
