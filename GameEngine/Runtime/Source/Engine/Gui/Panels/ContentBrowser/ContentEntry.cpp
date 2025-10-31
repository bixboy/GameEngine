#include "Engine/Gui/Panels/ContentBrowser/ContentEntry.h"

#include <array>

#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserFileUtils.h"

namespace BixEngine::Gui
{
    // ─────────────────────────────────────────────
    // 🧩  Génération de clé unique (pour ImGui)
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

    // ─────────────────────────────────────────────
    // 🖼️  Table d’icônes FontAwesome par type
    // ─────────────────────────────────────────────
    
    namespace
    {
        constexpr std::array<const char*, 6> kIcons {{
            "\xef\x81\xbc", // Folder
            "\xef\x87\x83", // File
            "\xef\x84\x9b", // Script
            "\xef\x8f\x88", // Actor prefab
            "\xef\x89\xb2", // Component prefab
            "\xef\x80\x8b"  // Sprite atlas (th icon)
        }};
    }

    const char* GetIcon(ContentType type)
    {
        const size_t index = static_cast<size_t>(type);
        return (index < kIcons.size()) ? kIcons[index] : kIcons[1];
    }

    // ─────────────────────────────────────────────
    // 🧭  Ordre de tri logique des entrées
    // ─────────────────────────────────────────────
    
    int GetSortPriority(ContentType type)
    {
        switch (type)
        {
        case ContentType::Directory:       return 0;
        case ContentType::ActorPrefab:     return 1;
        case ContentType::ComponentPrefab: return 2;
        case ContentType::Script:          return 3;
        case ContentType::SpriteAtlas:     return 4;
        case ContentType::File:
        default:                             return 5;
        }
    }
}
