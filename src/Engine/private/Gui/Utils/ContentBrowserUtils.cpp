#include "Gui/Utils/ContentBrowserUtils.h"
#include "Debug/Logger.h"
#include <unordered_map>
#include "Utils/FileIO/FilesUtils.h"
#include "Utils/String/StringUtils.h"
#include <algorithm>

namespace BixEngine::Gui::ContentBrowserUtils
{
    using namespace std;
    using namespace std::filesystem;

    namespace
    {
        int GetContentSortPriority(ContentType type)
        {
            switch (type)
            {
            case ContentType::Directory:
                return 0;
            case ContentType::Script:
                return 1;
            case ContentType::ActorPrefab:
                return 2;
            case ContentType::ComponentPrefab:
                return 3;
            case ContentType::SpriteAtlas:
                return 4;
            case ContentType::Audio:
                return 5;
            default:
                return 10;
            }
        }
    }

    ContentType DetectContentType(const path& path)
    {
        if (path.empty())
            return ContentType::File;

        const String ext = StringUtils::Utilities::ToLowerCopy(path.extension().generic_string());

        if (ext == ".bixactor")
            return ContentType::ActorPrefab;
        
        if (ext == ".bixcomponent")
            return ContentType::ComponentPrefab;
        
        if (ext == ".atlas")
            return ContentType::SpriteAtlas;
        
        if (ext == ".mp3" || ext == ".wav" || ext == ".ogg")
            return ContentType::Audio;

        return ContentType::File;
    }

    bool RefreshDirectoryCache(ContentBrowserState& state)
    {
        const bool needsRefresh = state.cache.dirty || state.cache.directory != state.current;
        if (!needsRefresh)
            return true;

        state.cache.directory = state.current;
        state.cache.entries.clear();
        state.cache.dirty = false;

        LOG_INFO("RefreshDirectoryCache: Enumerating " + state.current.generic_string());

        vector<directory_entry> fsEntries;
        std::error_code err;
        
        for (const auto& e : directory_iterator(state.current, err))
            fsEntries.emplace_back(e);

        if (err)
        {
            String message = String("Failed to enumerate content: " + err.message());
            Utils::FileUtils::LogAndStoreError(state.error, message);
            return false;
        }

        state.error.clear();
        state.cache.entries.reserve(fsEntries.size());

        unordered_map<String, ContentEntry> scriptGroups;

        for (const auto& entry : fsEntries)
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
            const bool isHeader = (ext == ".h" || ext == ".hpp"); 
            const bool isSource = (ext == ".cpp" || ext == ".cxx");

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

            const ContentType detectedType = DetectContentType(entryPath);
            
            ContentEntry e;
            e.type = detectedType;
            e.path = entryPath;
            e.name = entryPath.filename().generic_string();

            state.cache.entries.push_back(std::move(e));
        }

        for (auto& [key, entry] : scriptGroups)
        {
            state.cache.entries.push_back(std::move(entry));
        }

        std::ranges::sort(state.cache.entries, [](const ContentEntry& a, const ContentEntry& b)
        {
            int pa = GetContentSortPriority(a.type);
            int pb = GetContentSortPriority(b.type);
    
            if (pa != pb)
                return pa < pb;
        
            return Utils::FileUtils::CaseInsensitiveLess(a.name.c_str(), b.name.c_str());
        });

        return true;
    }

    void ClearSelectedParent(PopupRequestState& r)
    {
        r.selectedParentClass.clear();
        r.selectedParentInclude.clear();
        r.selectedParentDisplay.clear();
        r.selectedParentIsBase = false;
        r.selectedParentIsActor = false;
        r.selectedParentIsComponent = false;
    }

    void ClearSelectedPrefab(PopupRequestState& r)
    {
        r.selectedPrefabClass.clear();
        r.selectedPrefabInclude.clear();
        r.selectedPrefabAssetBase.clear();
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

        if (!Utils::FileUtils::TryCreateDir(dir, err))
            LOG_ERROR(String("Failed to create scripts directory: " + dir.string() ) + " (" + err + ")");
    }

    path GetContentRoot()
    {
        std::error_code cwdError;
        const path basePath = current_path(cwdError);

        if (cwdError)
        {
            auto message = String("Failed to determine working directory: ");
            message += cwdError.message().c_str();
            LOG_ERROR(message);
            return {};
        }

        path contentPath = basePath / "Content";
        if (exists(contentPath))
            return contentPath;

        path resourcesPath = basePath / "Resources";
        if (exists(resourcesPath))
            return resourcesPath;

        return contentPath;
    }

    bool EnsureContentBrowserInitialized(ContentBrowserState& state)
    {
        if (state.initialized)
            return state.error.empty();

        state.root = GetContentRoot();
        
        if (state.root.empty())
        {
            Utils::FileUtils::LogAndStoreError(state.error, "Unable to determine the Content directory root.");
        }
        else if (!Utils::FileUtils::TryCreateDir(state.root, state.error))
        {
            // Rien
        }
        else if (!exists(state.root))
        {
            String message = String("Content directory is not available: " + state.root.string());
            Utils::FileUtils::LogAndStoreError(state.error, std::move(message));
        }
        else
        {
            state.current = state.root;
            state.cache.directory.clear();
            state.cache.entries.clear();
            state.cache.dirty = true;
            state.error.clear();
        }

        state.initialized = true;
        return state.error.empty();
    }
}