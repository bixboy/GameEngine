#pragma once
#include <atomic>
#include <future>
#include <vector>
#include <filesystem>
#include "Containers/String.h"
#include "Gui/Panels/ContentBrowser/ContentEntry.h"
#include "Gui/Panels/ContentBrowser/ContentBrowserState.h"

namespace BixEngine::Gui
{
    class AsyncDirectoryLoader
    {
    public:
        AsyncDirectoryLoader();
        ~AsyncDirectoryLoader();

        void Request(const std::filesystem::path& directory);
        bool Tick(ContentBrowserState& state);

        bool IsBusy() const noexcept { return running_.load(); }

    private:
        struct Result
        {
            std::filesystem::path directory;
            std::vector<ContentEntry> entries;
            std::unordered_map<std::string, std::uintmax_t> sizes;
            std::unordered_map<std::string, std::filesystem::file_time_type> dates;
            String error;
        };

        std::atomic<bool> running_{false};
        std::atomic<uint64_t> ticket_{0};
        std::future<Result> future_;
        std::mutex mtx_;
        uint64_t currentTicket_{0};

        static Result Scan(const std::filesystem::path& directory);
    };
}
