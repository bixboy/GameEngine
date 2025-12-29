#pragma once
#include "Containers/String.h"
#include <filesystem>


namespace BixEngine::Gui
{
    struct ContentBrowserState;
    
    
    
    

    enum class ContentType
    {
        Directory = 0,
        File,
        Script,
        ActorPrefab,
        ComponentPrefab,
        SpriteAtlas,
        Audio,
    };

    
    
    

    struct ContentEntry
    {
        String name{};
        std::filesystem::path path{};
        ContentType type{ContentType::File};
        String extension{};

        std::filesystem::path headerPath{};
        std::filesystem::path sourcePath{};

        
        
        

        [[nodiscard]] bool IsDirectory() const noexcept { return type == ContentType::Directory; }
        [[nodiscard]] bool IsFile() const noexcept { return type == ContentType::File; }
        [[nodiscard]] bool IsScript() const noexcept { return type == ContentType::Script; }
        [[nodiscard]] bool IsActorPrefab() const noexcept { return type == ContentType::ActorPrefab; }
        [[nodiscard]] bool IsComponentPrefab() const noexcept { return type == ContentType::ComponentPrefab; }
        [[nodiscard]] bool IsSpriteAtlas() const noexcept { return type == ContentType::SpriteAtlas; }
        [[nodiscard]] bool IsAudio() const noexcept { return type == ContentType::Audio; }
        [[nodiscard]] bool IsAudioContainer() const noexcept { return extension == ".bixaudio"; }
        [[nodiscard]] bool IsPrefab() const noexcept { return IsActorPrefab() || IsComponentPrefab(); }

        [[nodiscard]] bool HasHeader() const noexcept { return !headerPath.empty(); }
        [[nodiscard]] bool HasSource() const noexcept { return !sourcePath.empty(); }

        [[nodiscard]] bool IsAsset() const noexcept { return IsPrefab() || IsScript() || IsSpriteAtlas() || IsAudio(); }

        [[nodiscard]] String SelectionKey() const;
    };

    
    
    

    const char* GetIcon(ContentType type);
    int GetSortPriority(ContentType type);

    bool DrawEntryButton(const ContentEntry& entry, bool isSelected, ContentBrowserState& state, String& selectedEntry);
    void DrawEntryTooltip(const ContentEntry& entry);
    void DrawEntryLabel(const ContentEntry& entry, bool isSelected);
}
