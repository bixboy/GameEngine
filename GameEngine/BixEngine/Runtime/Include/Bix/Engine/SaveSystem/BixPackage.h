#pragma once

#include <filesystem>
#include <memory>

#include "Bix/Engine/SaveSystem/BixArchive.h"

namespace BixEngine::Engine::SaveSystem
{
    class BixPackage
    {
    public:
        BixPackage() = default;
        explicit BixPackage(std::unique_ptr<BixObject> root);

        [[nodiscard]] BixObject* GetRoot() noexcept { return root_.get(); }
        [[nodiscard]] const BixObject* GetRoot() const noexcept { return root_.get(); }
        std::unique_ptr<BixObject> ReleaseRoot() noexcept { return std::move(root_); }

        static bool Save(const BixObject& root, const std::filesystem::path& filePath);
        static std::unique_ptr<BixPackage> Load(const std::filesystem::path& filePath);

    private:
        std::unique_ptr<BixObject> root_;
    };
}

