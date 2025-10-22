#include "Bix/Engine/SaveSystem/BixSaveSystem.h"

#include <fstream>
#include <stdexcept>
#include <vector>

#include "Bix/Engine/SaveSystem/BixPackage.h"

namespace BixEngine::Engine::SaveSystem
{
    namespace
    {
        void CollectPropertiesRecursive(const BixClass& cls, std::vector<const BixProperty*>& out)
        {
            if (const BixClass* super = cls.GetSuperClass())
                CollectPropertiesRecursive(*super, out);

            for (const BixProperty& property : cls.GetProperties())
                out.push_back(&property);
        }
    }

    bool BixSaveSystem::SavePackage(const BixObject& root, const std::filesystem::path& filePath)
    {
        return BixPackage::Save(root, filePath);
    }

    std::unique_ptr<BixObject> BixSaveSystem::LoadPackage(const std::filesystem::path& filePath)
    {
        auto package = BixPackage::Load(filePath);
        if (!package)
            return nullptr;
        return package->ReleaseRoot();
    }

    std::unique_ptr<BixObject> BixSaveSystem::CreateInstance(std::string_view className)
    {
        return ClassRegistry::Get().CreateInstance(className);
    }

    void BixSaveSystem::EnumerateClasses(const std::function<void(const BixClass&)>& visitor)
    {
        ClassRegistry::Get().ForEachClass(visitor);
    }

    void SerializeObject(BixArchiveWriter& writer, const BixObject& object)
    {
        const BixClass& cls = object.GetClass();
        writer.WriteString(cls.GetName());
        writer.WriteStdString(cls.GetNativeName());
        writer.WriteGuid(object.GetGuid());

        const BixObject* outer = object.GetOuter();
        const bool hasOuter = outer != nullptr;
        writer.WritePrimitive(hasOuter);
        if (hasOuter)
            writer.WriteGuid(outer->GetGuid());

        std::vector<const BixProperty*> properties;
        CollectPropertiesRecursive(cls, properties);
        const auto propertyCount = static_cast<std::uint32_t>(properties.size());
        writer.WritePrimitive(propertyCount);

        for (const BixProperty* property : properties)
        {
            writer.WriteString(property->name);
            property->write(object, writer);
        }
    }

    std::unique_ptr<BixObject> DeserializeObject(BixArchiveReader& reader, BixObject* outer)
    {
        const String className = reader.ReadString();
        const std::string nativeName = reader.ReadStdString();

        const BixClass* cls = ClassRegistry::Get().FindClassByNative(nativeName);
        if (!cls)
            cls = ClassRegistry::Get().FindClass(className.Std());
        if (!cls)
            throw std::runtime_error("BixSaveSystem: Unknown class encountered while loading package.");

        auto instance = cls->CreateInstance();
        if (!instance)
            throw std::runtime_error("BixSaveSystem: Unable to instantiate class.");

        const BixGuid guid = reader.ReadGuid();
        instance->SetGuid(guid);
        instance->SetOuter(outer);

        bool hasOuter = false;
        reader.ReadPrimitive(hasOuter);
        if (hasOuter)
        {
            const BixGuid outerGuid = reader.ReadGuid();
            if (outer && outer->GetGuid() != outerGuid)
                throw std::runtime_error("BixSaveSystem: Outer GUID mismatch while loading object.");
        }

        std::vector<const BixProperty*> properties;
        CollectPropertiesRecursive(*cls, properties);

        std::uint32_t propertyCount = 0;
        reader.ReadPrimitive(propertyCount);

        for (std::uint32_t i = 0; i < propertyCount; ++i)
        {
            const String propertyName = reader.ReadString();
            const BixProperty* target = nullptr;
            for (const BixProperty* property : properties)
            {
                if (property->name == propertyName)
                {
                    target = property;
                    break;
                }
            }

            if (!target)
                throw std::runtime_error("BixSaveSystem: Unknown property encountered while loading object.");

            target->read(*instance, reader);
        }

        instance->OnPostDeserialize();

        return instance;
    }
}

