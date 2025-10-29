#pragma once
#include "Core/Containers/String.h"


namespace BixEngine::Gui
{
    struct ContentBrowserState;
    struct PopupRequestState;

    void RenderHeader(ContentBrowserState& state, String& selectedEntry, char (&searchBuffer)[256]);
}

