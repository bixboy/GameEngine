#include "Engine/Gui/Panels/ContentBrowser/ContentEntry.h"

#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserFileUtils.h"

namespace BixEngine::Gui
{
    // ─────────────────────────────────────────────
    // 🧱  Gestion des métadonnées d'entrée
    // ─────────────────────────────────────────────

    String ContentEntry::SelectionKey() const
    {
        if (path.empty())
            return name;

        std::filesystem::path key = path;
        if (IsScript())
            key /= name.View();
        return key.generic_string();
    }

    const char* GetIcon(ContentType type)
    {
        switch (type)
        {
            case ContentType::Directory:
                return "\xef\x81\xbc"; // Folder icon
            case ContentType::Script:
                return "\xef\x84\x9b"; // File-code icon
            case ContentType::Actor:
                return "\xef\x8f\x88"; // Cube icon
            case ContentType::File:
            default:
                return "\xef\x87\x83"; // File icon
        }
    }

    int GetSortPriority(ContentType type)
    {
        switch (type)
        {
            case ContentType::Directory:
                return 0;
            case ContentType::Actor:
                return 1;
            case ContentType::Script:
                return 2;
            case ContentType::File:
            default:
                return 3;
        }
    }
}

