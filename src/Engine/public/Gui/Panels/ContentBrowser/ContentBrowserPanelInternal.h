#pragma once
#include "Gui/Panels/ContentBrowser/ContentBrowserState.h"


namespace BixEngine::Gui
{
    struct PopupRequestState;

    void RenderHeader(ContentBrowserState& state, String& selectedEntry, char (&searchBuffer)[256]);

    void RenderDirectoryTree(ContentBrowserState& state, String& selectedEntry);

    void RenderEntries(ContentBrowserState& state, String& selectedEntry, PopupRequestState& requestPopups, const String& searchQuery);

    void RenderPopups(ContentBrowserState& state, String& selectedEntry, PopupRequestState& requestPopups);

    
    bool RefreshDirectoryCache(ContentBrowserState& state);
    bool DeleteScriptFiles(const ContentEntry& entry, String& error);
}
