#include "Engine/Gui/Panels/ContentBrowser/Utils/ContentBrowserAsyncLoader.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserFileUtils.h"
#include <system_error>

namespace BixEngine::Gui
{
    AsyncDirectoryLoader::AsyncDirectoryLoader() = default;
    AsyncDirectoryLoader::~AsyncDirectoryLoader()
    {
        // on laisse le future se terminer tout seul si besoin
    }

    void AsyncDirectoryLoader::Request(const std::filesystem::path& directory)
    {
        const uint64_t myTicket = ++ticket_;
        running_.store(true);
        future_ = std::async(std::launch::async, [myTicket, directory]()
        {
            (void)myTicket;
            return Scan(directory);
        });
        std::scoped_lock lk(mtx_);
        currentTicket_ = myTicket;
    }

    bool AsyncDirectoryLoader::Tick(ContentBrowserState& state)
    {
        if (!running_.load())
            return false;

        using namespace std::chrono_literals;
        // try_status pour ne pas bloquer
        if (future_.valid() && future_.wait_for(0ms) == std::future_status::ready)
        {
            auto res = future_.get();
            running_.store(false);

            if (!res.error.IsEmpty())
            {
                LogAndStoreError(state.error, std::move(res.error));
                return true;
            }

            // Appliquer au cache
            state.cache.directory = res.directory;
            state.cache.entries = std::move(res.entries);
            state.cache.fileSizes = std::move(res.sizes);
            state.cache.lastWriteTimes = std::move(res.dates);
            state.cache.dirty = false;
            state.error.Clear();
            return true;
        }
        return false;
    }

    AsyncDirectoryLoader::Result AsyncDirectoryLoader::Scan(const std::filesystem::path& directory)
    {
        namespace fs = std::filesystem;
        Result r;
        r.directory = directory;

        std::vector<fs::directory_entry> entries;
        std::error_code iterationError;
        for (const auto& e : fs::directory_iterator(directory, iterationError))
            entries.emplace_back(e);
        if (iterationError)
        {
            r.error = String("Failed to enumerate content: ") + iterationError.message();
            return r;
        }

        std::unordered_map<String, ContentEntry> scriptGroups;

        for (const auto& e : entries)
        {
            const fs::path p = e.path();

            if (e.is_directory())
            {
                ContentEntry ce{};
                ce.type = ContentType::Directory;
                ce.path = p;
                ce.name = p.filename().generic_string();
                r.entries.push_back(std::move(ce));
                continue;
            }

            // métadonnées (taille/date) — best-effort
            std::error_code ecSize, ecTime;
            const auto key = p.generic_string();
            if (e.is_regular_file())
                r.sizes[key] = fs::file_size(p, ecSize);
            r.dates[key] = fs::last_write_time(p, ecTime);

            const fs::path ext = p.extension();
            const bool isHeader = ext == ".h";
            const bool isSource = ext == ".cpp";
            if (isHeader || isSource)
            {
                String gk = ToLowerCopy(p.stem().generic_string());
                auto& grp = scriptGroups[gk];
                grp.type = ContentType::Script;
                grp.path = p.parent_path();
                if (grp.name.IsEmpty())
                    grp.name = p.stem().generic_string();
                if (isHeader) grp.headerPath = p; else grp.sourcePath = p;
                continue;
            }

            const String extensionLower = ToLowerCopy(p.extension().generic_string());
            if (extensionLower == ".bixactor")
            {
                ContentEntry ce{};
                ce.type = ContentType::ActorPrefab;
                ce.path = p;
                ce.name = p.filename().generic_string();
                r.entries.push_back(std::move(ce));
                continue;
            }

            if (extensionLower == ".bixcomponent")
            {
                ContentEntry ce{};
                ce.type = ContentType::ComponentPrefab;
                ce.path = p;
                ce.name = p.filename().generic_string();
                r.entries.push_back(std::move(ce));
                continue;
            }

            ContentEntry fe{};
            fe.type = ContentType::File;
            fe.path = p;
            fe.name = p.filename().generic_string();
            r.entries.push_back(std::move(fe));
        }

        for (auto& [_, sc] : scriptGroups)
            r.entries.push_back(std::move(sc));

        // Tri par défaut: type -> nom
        std::ranges::sort(r.entries, [](const ContentEntry& a, const ContentEntry& b)
        {
            const int ap = GetSortPriority(a.type);
            const int bp = GetSortPriority(b.type);
            if (ap != bp) return ap < bp;
            return CaseInsensitiveLess(a.name, b.name);
        });

        return r;
    }
}
