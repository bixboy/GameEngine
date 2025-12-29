#include "Gui/Panels/ContentBrowser/ContentBrowserPanelInternal.h"
#include "Gui/Panels/ContentBrowser/ContentBrowserState.h"
#include "Gui/Panels/ContentBrowser/ContentEntry.h"
#include "Utils/FileIO/FilesUtils.h"
#include "Utils/String/StringUtils.h"
#include "Debug/Logger.h"

#include <filesystem>
#include <unordered_map>
#include <ranges>
#include <algorithm>

namespace BixEngine::Gui
{
    namespace fs = std::filesystem;

    
    
    

    bool DeleteScriptFiles(const ContentEntry& entry, String& error)
    {
        auto tryRemove = [&](const std::filesystem::path& p)
        {
            return p.empty() || FilesUtils::Utilities::TryRemove(p, false, error);
        };

        return tryRemove(entry.headerPath) && tryRemove(entry.sourcePath);
    }

    
    
    

   bool RefreshDirectoryCache(ContentBrowserState& state)
    {
        
        if (!state.cache.dirty && state.cache.directory == state.current)
            return true;

        
        
        std::vector<ContentEntry> newEntries;
        std::unordered_map<String, ContentEntry> scriptGroups;
        
        std::error_code error;

        LOG_INFO("Refreshing directory cache for: " + state.current.generic_string());

        
        try 
        {
            
            if (!fs::exists(state.current)) 
                return false;

            for (auto& entry : fs::directory_iterator(state.current, error))
            {
                if (error) {
                    
                    LOG_WARNING("Filesystem error during iteration: " + error.message());
                    break; 
                }

                const std::filesystem::path p = entry.path();

                
                if (entry.is_directory())
                {
                    newEntries.push_back(ContentEntry{
                        .name = p.filename().generic_string(),
                        .path = p,
                        .type = ContentType::Directory
                    });
                    continue;
                }

                
                const String ext = StringUtils::Utilities::ToLowerCopy(p.extension().generic_string());

                
                if (ext == ".h" || ext == ".cpp")
                {
                    auto key = StringUtils::Utilities::ToLowerCopy(p.stem().generic_string());
                    auto& group = scriptGroups[key];

                    
                    if (group.name.empty()) {
                        group.name = p.stem().generic_string();
                        group.path = p.parent_path();
                        group.type = ContentType::Script;
                    }

                    if (ext == ".h") group.headerPath = p;
                    else group.sourcePath = p;

                    continue;
                }

                
                ContentType type = ContentType::File;

                if (ext == ".bixactor") type = ContentType::ActorPrefab;
                else if (ext == ".bixcomponent") type = ContentType::ComponentPrefab;
                else if (ext == ".atlas") type = ContentType::SpriteAtlas;
                else if (ext == ".mp3" || ext == ".wav" || ext == ".ogg") type = ContentType::Audio;

                newEntries.push_back(ContentEntry{
                    .name = p.filename().generic_string(),
                    .path = p,
                    .type = type,
                    .extension = ext
                });
                LOG_INFO("Found file: " + p.filename().generic_string());
            }
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Crash prevented in Content Browser: " + std::string(e.what()));
            return false; 
        }

        
        for (auto& script : scriptGroups | std::views::values)
        {
            newEntries.push_back(std::move(script));   
        }

        
        std::ranges::sort(newEntries, [](const ContentEntry& a, const ContentEntry& b)
        {
            int pa = GetSortPriority(a.type);
            int pb = GetSortPriority(b.type);

            if (pa != pb) return pa < pb;
            return FilesUtils::Utilities::CaseInsensitiveLess(a.name, b.name);
        });

        
        
        state.cache.entries = std::move(newEntries);
        
        
        state.cache.dirty = false;
        state.cache.directory = state.current;

        return !error;
    }
}
