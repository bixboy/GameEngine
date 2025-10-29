#pragma once

#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserState.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentEntry.h"

#include "imgui.h"

namespace BixEngine::Gui
{
    struct PopupRequestState;

    void RenderHeader(ContentBrowserState& state, String& selectedEntry, char (&searchBuffer)[256]);
    void RenderDirectoryTree(ContentBrowserState& state, String& selectedEntry);
    void RenderEntries(ContentBrowserState& state, String& selectedEntry, PopupRequestState& requestPopups, const String& searchQuery);

    void RenderPopups(ContentBrowserState& state, String& selectedEntry, PopupRequestState& requestPopups);

    void RunBixHeaderTool(const std::filesystem::path& toolPath, const std::filesystem::path& headerPath);

    inline constexpr ImVec4 kContentBackground{0.09f, 0.09f, 0.09f, 0.95f};
}

