#pragma once

#include "Core/Containers/String.h"

#include <functional>
#include <vector>

namespace BixEngine::Gui
{
    struct ContentBrowserState;
    struct ContentEntry;
    struct PopupRequestState;

    struct EntryAction
    {
        const char* label{nullptr};
        std::function<void(ContentBrowserState&, const ContentEntry&, PopupRequestState&, String&)> callback{};
        bool enabled{true};
    };

    using EntryActionList = std::vector<EntryAction>;

    EntryActionList BuildActionsFor(const ContentBrowserState& state, const ContentEntry& entry);
    void DrawEntryContextMenu(ContentBrowserState& state, const ContentEntry& entry, PopupRequestState& requests, String& selectionKey);
}

