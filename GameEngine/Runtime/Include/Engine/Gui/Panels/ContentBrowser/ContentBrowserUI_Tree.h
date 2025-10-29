#pragma once

#include "Core/Containers/String.h"

namespace BixEngine::Gui
{
    struct ContentBrowserState;

    void RenderDirectoryTree(ContentBrowserState& state, String& selectedEntry);
}

