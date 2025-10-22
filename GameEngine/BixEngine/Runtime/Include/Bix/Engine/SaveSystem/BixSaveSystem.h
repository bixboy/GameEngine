#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <vector>

#include "Bix/Engine/SaveSystem/BixArchive.h"
#include "Bix/Engine/SaveSystem/BixReflection.h"

namespace BixEngine::Engine::SaveSystem
{
    class BixPackage;

    class BixSaveSystem
    {
    public:
        static bool SavePackage(const BixObject& root, const std::filesystem::path& filePath);
        static std::unique_ptr<BixObject> LoadPackage(const std::filesystem::path& filePath);

        template<typename TObject>
        static bool SaveObject(const TObject& object, const std::filesystem::path& filePath)
        {
            return SavePackage(object, filePath);
        }

        template<typename TObject>
        static std::unique_ptr<TObject> LoadObject(const std::filesystem::path& filePath)
        {
            auto object = LoadPackage(filePath);
            return std::unique_ptr<TObject>(static_cast<TObject*>(object.release()));
        }

        static std::unique_ptr<BixObject> CreateInstance(std::string_view className);
        static void EnumerateClasses(const std::function<void(const BixClass&)>& visitor);
    };
}

