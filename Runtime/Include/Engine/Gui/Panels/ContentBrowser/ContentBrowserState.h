#pragma once

#include "Core/Containers/String.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentEntry.h"

#include <filesystem>
#include <functional>
#include <imgui.h>
#include <vector>

namespace BixEngine::Gui
{
    enum class ContentViewMode
    {
        Grid = 0, List
    };
    enum class ContentSortMode
    {
        TypeThenName = 0,
        NameAZ,
        NameZA,
        DateNewest,
        DateOldest
    };
    
    enum class ScriptTemplateType
    {
        Actor = 0,
        Component,
        Utility,
    };

    struct PopupRequestState
    {
        bool createScript{false};
        bool createPrefab{false};
        bool createFolder{false};
        bool createSpriteAtlas{false};
        bool renameEntry{false};

        ScriptTemplateType scriptType{ScriptTemplateType::Actor};
        char scriptName[128] = "NewScript";
        char folderName[128] = "NewFolder";
        char spriteAtlasTexturePath[260] = "";
        char renameBuffer[256] = "";
        String scriptError{};
        String prefabError{};
        String folderError{};
        String spriteAtlasError{};
        String renameError{};
        std::filesystem::path folderTarget{};
        std::filesystem::path spriteAtlasTarget{};
        bool spriteAtlasBrowseTextures{false};
        std::filesystem::path renameTarget{};
        std::filesystem::path renameSecondaryTarget{};
        bool renameTargetIsScriptGroup{false};
        String selectedParentClass{};
        String selectedParentInclude{};
        String selectedParentDisplay{};
        bool selectedParentIsBase{false};
        bool selectedParentIsActor{false};
        bool selectedParentIsComponent{false};

        String selectedPrefabClass{};
        String selectedPrefabInclude{};
        String selectedPrefabAssetBase{};
        std::filesystem::path selectedPrefabScript{};
        bool selectedPrefabIsActor{false};
        bool selectedPrefabIsComponent{false};

        int spriteAtlasColumns{1};
        int spriteAtlasRows{1};
        int spriteAtlasPadding{0};
        int spriteAtlasMargin{0};
    };

    struct DirectoryCache
    {
        std::filesystem::path directory{};
        std::vector<ContentEntry> entries{};
        bool dirty{true};

        std::unordered_map<std::string, std::uintmax_t> fileSizes;
        std::unordered_map<std::string, std::filesystem::file_time_type> lastWriteTimes;
        void ClearMeta() { fileSizes.clear(); lastWriteTimes.clear(); }
    };

    struct ContentBrowserState
    {
        std::filesystem::path root{};
        std::filesystem::path current{};
        String error{};
        bool initialized{false};
        std::function<void(const std::vector<std::filesystem::path>&)> openScriptFilesCallback{};
        std::function<void(const std::filesystem::path&)> openAssetEditorCallback{};

        std::function<ImTextureID(const std::filesystem::path&)> getPreviewTextureCallback{};
        ContentViewMode viewMode{ContentViewMode::Grid};
        ContentSortMode sortMode{ContentSortMode::TypeThenName};
        float gridZoom{1.0f};

        DirectoryCache cache{};
    };

    void ClearSelectedParent(PopupRequestState& requests);
    void ClearSelectedPrefab(PopupRequestState& requests);

    bool EnsureContentBrowserInitialized(ContentBrowserState& state);
    void EnsureScriptsDirectoryExists(const ContentBrowserState& state);
}

