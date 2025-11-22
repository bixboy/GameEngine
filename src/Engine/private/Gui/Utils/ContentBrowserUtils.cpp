#include "Gui/Utils//ContentBrowserUtils.h"
#include "Logger.h"
#include <ranges>
#include <unordered_map>

#include "Utils/FilesUtils.h"
#include "Utils/StringUtils.h"

namespace BixEngine::Gui::ContentBrowserUtils
{
    using namespace std;
    using namespace std::filesystem;

    ContentType DetectContentType(const path& path)
    {
        if (path.empty()) return ContentType::File;

        const String ext = StringUtils::Utilities::ToLowerCopy(path.extension().generic_string());

        if (ext == ".bixactor")
            return ContentType::ActorPrefab;

        if (ext == ".bixcomponent")
            return ContentType::ComponentPrefab;

        if (ext == ".atlas")
            return ContentType::SpriteAtlas;

        if (ext == ".atlas")
            return ContentType::SpriteAtlas;

        if (ext == ".mp3" || ext == ".wav" || ext == ".ogg")
            return ContentType::Audio;

        return ContentType::File;
    }

    bool RefreshDirectoryCache(ContentBrowserState& state)
    {
        namespace fs = std::filesystem;

        const bool needsRefresh = state.cache.dirty || state.cache.directory != state.current;
        if (!needsRefresh)
            return true;

        state.cache.directory = state.current;
        state.cache.entries.clear();
        state.cache.dirty = false;

        LOG_INFO("RefreshDirectoryCache: Enumerating " + state.current.generic_string());

        vector<directory_entry> entries;
        std::error_code err;
        for (const auto& e : directory_iterator(state.current, err))
            entries.emplace_back(e);

        if (err)
        {
            String message = String("Failed to enumerate content: ") + err.message();
            FilesUtils::Utilities::LogAndStoreError(state.error, message);
            return false;
        }

        state.error.Clear();
        state.cache.entries.reserve(entries.size());

        unordered_map<String, ContentEntry> scriptGroups;

        for (const auto& entry : entries)
        {
            const path& entryPath = entry.path();

            if (entry.is_directory())
            {
                ContentEntry d;
                d.type = ContentType::Directory;
                d.path = entryPath;
                d.name = entryPath.filename().generic_string();
                state.cache.entries.push_back(std::move(d));
                continue;
            }

            const auto ext = entryPath.extension();
            const bool isHeader = ext == ".h";
            const bool isSource = ext == ".cpp";

            if (isHeader || isSource)
            {
                String key = StringUtils::Utilities::ToLowerCopy(entryPath.stem().generic_string());

                auto& group = scriptGroups[key];
                if (group.type != ContentType::Script)
                {
                    group.type = ContentType::Script;
                    group.path = entryPath.parent_path();
                    group.name = entryPath.stem().generic_string();
                }

                if (isHeader)
                    group.headerPath = entryPath;
                else
                    group.sourcePath = entryPath;

                continue;
            }

            const ContentType prefabType = DetectContentType(entryPath);

            ContentEntry e;
            e.type = prefabType;
            e.path = entryPath;
            e.name = entryPath.filename().generic_string();

            state.cache.entries.push_back(std::move(e));
        }

        for (auto& [_, s] : scriptGroups)
        {
            state.cache.entries.push_back(std::move(s));
        }

        std::ranges::sort(state.cache.entries, [](const ContentEntry& a, const ContentEntry& b)
        {
            int pa = GetSortPriority(a.type), pb = GetSortPriority(b.type);
            return pa == pb ? FilesUtils::Utilities::CaseInsensitiveLess(a.name, b.name) : pa < pb;
        });

        LOG_INFO("RefreshDirectoryCache: Done. Found " + std::to_string(state.cache.entries.size()) + " entries.");
        return true;
    }

    int GetSortPriority(ContentType type)
    {
        switch (type)
        {
        case ContentType::Directory: return 0;
        case ContentType::Script: return 1;
        case ContentType::ActorPrefab: return 2;
        case ContentType::ComponentPrefab: return 3;
        case ContentType::SpriteAtlas: return 4;
        case ContentType::Audio: return 5;
        default: return 10;
        }
    }

    void ClearSelectedParent(PopupRequestState& r)
    {
        r.selectedParentClass.Clear();
        r.selectedParentInclude.Clear();
        r.selectedParentDisplay.Clear();
        r.selectedParentIsBase = false;
        r.selectedParentIsActor = false;
        r.selectedParentIsComponent = false;
    }

    void ClearSelectedPrefab(PopupRequestState& r)
    {
        r.selectedPrefabClass.Clear();
        r.selectedPrefabInclude.Clear();
        r.selectedPrefabAssetBase.Clear();
        r.selectedPrefabScript.clear();
        r.selectedPrefabIsActor = false;
        r.selectedPrefabIsComponent = false;
    }

    void EnsureScriptsDirectoryExists(const ContentBrowserState& state)
    {
        if (state.root.empty())
            return;

        const path dir = state.root / "Scripts";
        String err;

        if (!FilesUtils::Utilities::TryCreateDir(dir, err))
            LOG_ERROR(String("Failed to create scripts directory: ") + dir.string() + " (" + err + ")");
    }

    path GetContentRoot()
    {
        namespace fs = std::filesystem;

        std::error_code cwdError;
        const path basePath = current_path(cwdError);

        if (cwdError)
        {
            auto message = String("Failed to determine working directory: ");
            message += cwdError.message();
            LOG_ERROR(message);

            return {};
        }

        const path contentPath = basePath / "Content";
        if (exists(contentPath))
            return contentPath;

        const path resourcesPath = basePath / "Resources";
        if (exists(resourcesPath))
            return resourcesPath;

        return contentPath;
    }

    bool EnsureContentBrowserInitialized(ContentBrowserState& state)
    {
        namespace fs = std::filesystem;

        if (state.initialized)
            return state.error.IsEmpty();

        state.root = GetContentRoot();
        if (state.root.empty())
        {
            FilesUtils::Utilities::LogAndStoreError(state.error, "Unable to determine the Content directory root.");
        }
        else if (!FilesUtils::Utilities::TryCreateDir(state.root, state.error))
        {
            // Error already stored.
        }
        else if (!exists(state.root))
        {
            String message = String("Content directory is not available: ") + state.root.string();
            FilesUtils::Utilities::LogAndStoreError(state.error, std::move(message));
        }
        else
        {
            state.current = state.root;
            state.cache.directory.clear();
            state.cache.entries.clear();
            state.cache.dirty = true;
            state.error.Clear();
        }

        state.initialized = true;
        return state.error.IsEmpty();
    }
}
