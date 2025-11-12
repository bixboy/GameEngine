#pragma once
#include "Gui/Panels/ContentBrowser/ContentBrowserState.h"
#include "Gui/Panels/ContentBrowser/ContentEntry.h"


namespace BixEngine::Gui::ContentBrowserUtils
{
    ContentType DetectContentType(const std::filesystem::path& path);

    bool RefreshDirectoryCache(ContentBrowserState& state);

    void ClearSelectedParent(PopupRequestState& requests);

    void ClearSelectedPrefab(PopupRequestState& requests);

    std::filesystem::path GetContentRoot();

    void EnsureScriptsDirectoryExists(const ContentBrowserState& state);

    bool EnsureContentBrowserInitialized(ContentBrowserState& state);
}
