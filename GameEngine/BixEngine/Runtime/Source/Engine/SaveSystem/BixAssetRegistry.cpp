#include "Bix/Engine/SaveSystem/BixAssetRegistry.h"

#include <fstream>

#include "Bix/Engine/SaveSystem/BixArchive.h"

namespace BixEngine::Engine::SaveSystem
{
    namespace
    {
        constexpr const char* kAssetExtension = ".bixasset";

        bool IsBixAssetFile(const std::filesystem::path& path)
        {
            return path.has_extension() && path.extension() == kAssetExtension;
        }

        bool ReadRootClassInfo(const std::filesystem::path& path, String& className, std::string& nativeName)
        {
            std::ifstream file(path, std::ios::binary);
            if (!file.is_open())
                return false;

            BixArchiveReader reader(file);
            const std::string signature = reader.ReadStdString();
            if (signature != "BIXASSET")
                return false;

            std::uint32_t version = 0;
            reader.ReadPrimitive(version);
            if (version != 1)
                return false;

            className = reader.ReadString();
            nativeName = reader.ReadStdString();
            return true;
        }
    }

    BixAssetRegistry& BixAssetRegistry::Get()
    {
        static BixAssetRegistry registry;
        return registry;
    }

    void BixAssetRegistry::ScanContentDirectory(const std::filesystem::path& rootDirectory)
    {
        assets_.clear();
        if (!std::filesystem::exists(rootDirectory))
            return;

        for (const auto& entry : std::filesystem::recursive_directory_iterator(rootDirectory))
        {
            if (!entry.is_regular_file())
                continue;

            const auto& path = entry.path();
            if (!IsBixAssetFile(path))
                continue;

            String className;
            std::string nativeName;
            if (!ReadRootClassInfo(path, className, nativeName))
                continue;

            assets_.push_back(BixAssetRecord{ path, className, nativeName });
        }
    }

    std::vector<BixAssetRecord> BixAssetRegistry::GetAssetsByClass(std::string_view className) const
    {
        std::vector<BixAssetRecord> results;
        for (const auto& asset : assets_)
        {
            if (asset.className == className)
                results.push_back(asset);
        }
        return results;
    }
}

