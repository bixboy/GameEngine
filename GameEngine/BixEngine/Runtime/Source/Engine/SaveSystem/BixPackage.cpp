#include "Bix/Engine/SaveSystem/BixPackage.h"

#include <fstream>
#include <stdexcept>

namespace BixEngine::Engine::SaveSystem
{
    namespace
    {
        constexpr const char* kPackageSignature = "BIXASSET";
        constexpr std::uint32_t kPackageVersion = 1;
    }

    BixPackage::BixPackage(std::unique_ptr<BixObject> root) : root_(std::move(root)) {}

    bool BixPackage::Save(const BixObject& root, const std::filesystem::path& filePath)
    {
        std::ofstream file(filePath, std::ios::binary);
        if (!file.is_open())
            return false;

        BixArchiveWriter writer(file);
        writer.WriteStdString(kPackageSignature);
        writer.WritePrimitive(kPackageVersion);
        SerializeObject(writer, root);
        return writer.Good();
    }

    std::unique_ptr<BixPackage> BixPackage::Load(const std::filesystem::path& filePath)
    {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open())
            return nullptr;

        BixArchiveReader reader(file);
        const std::string signature = reader.ReadStdString();
        if (signature != kPackageSignature)
            throw std::runtime_error("Invalid BIX package signature.");

        std::uint32_t version = 0;
        reader.ReadPrimitive(version);
        if (version != kPackageVersion)
            throw std::runtime_error("Unsupported BIX package version.");

        auto root = DeserializeObject(reader, nullptr);
        if (!root)
            return nullptr;

        return std::make_unique<BixPackage>(std::move(root));
    }
}

