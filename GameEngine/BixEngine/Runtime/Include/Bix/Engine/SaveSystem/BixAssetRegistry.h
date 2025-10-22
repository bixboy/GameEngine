#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "Bix/Core/String.h"

namespace BixEngine::Engine::SaveSystem
{
    struct BixAssetRecord
    {
        std::filesystem::path path;
        String className;
        std::string nativeClassName;
    };

    class BixAssetRegistry
    {
    public:
        static BixAssetRegistry& Get();

        void ScanContentDirectory(const std::filesystem::path& rootDirectory);
        [[nodiscard]] const std::vector<BixAssetRecord>& GetAssets() const noexcept { return assets_; }
        std::vector<BixAssetRecord> GetAssetsByClass(std::string_view className) const;

    private:
        std::vector<BixAssetRecord> assets_;
    };
}

