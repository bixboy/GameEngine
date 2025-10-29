#pragma once

#include "Core/Containers/String.h"

#include <filesystem>
#include <string_view>

namespace BixEngine::Gui
{
    enum class ContentType
    {
        Directory = 0,
        File,
        Script,
        Actor,
    };

    struct ContentEntry
    {
        String name{};
        std::filesystem::path path{};
        ContentType type{ContentType::File};

        std::filesystem::path headerPath{};
        std::filesystem::path sourcePath{};

        [[nodiscard]] bool IsDirectory() const noexcept { return type == ContentType::Directory; }
        [[nodiscard]] bool IsFile() const noexcept { return type == ContentType::File; }
        [[nodiscard]] bool IsScript() const noexcept { return type == ContentType::Script; }
        [[nodiscard]] bool IsActor() const noexcept { return type == ContentType::Actor; }
        [[nodiscard]] bool HasHeader() const noexcept { return !headerPath.empty(); }
        [[nodiscard]] bool HasSource() const noexcept { return !sourcePath.empty(); }
        [[nodiscard]] String SelectionKey() const;
    };

    const char* GetIcon(ContentType type);
    int GetSortPriority(ContentType type);
}

