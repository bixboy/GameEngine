#pragma once
#include "Containers/String.h"
#include "Core/PropertyInfo.h"
#include "Ressources/Core/ResourceManager.h"
#include <string>
#include <memory>


namespace BixEngine::Utils
{
    using namespace Bix::Reflection;
    struct GenericSubclassType {};

    // vérifie les noms de types
    inline bool IsResourceType(const std::string& typeName, const std::string& resourceName)
    {
        return typeName.find(resourceName) != std::string::npos;
    }

    // Récupère le chemin d'une ressource depuis une propriété (gère T* et shared_ptr<T>)
    template <typename TResource>
    String GetResourcePath(const PropertyInfo& prop, const void* instance)
    {
        if (prop.TypeName.find("*") != std::string::npos)
        {
            const auto* ptr = prop.Get<TResource*>(instance);
            return ptr ? ptr->GetPath() : String("");
        }
        
        const auto& ptr = prop.Get<std::shared_ptr<TResource>>(instance);
        return ptr ? ptr->GetPath() : String("");
    }

    // Charge et assigne une ressource depuis un chemin
    template <typename TResource>
    void SetResourceFromPath(const PropertyInfo& prop, void* instance, const String& path)
    {
        auto resource = Resources::ResourceManager::Get().Get<TResource>(path);

        if (prop.TypeName.find("*") != std::string::npos)
        {
            const_cast<PropertyInfo&>(prop).Get<TResource*>(instance) = resource.get();
        }
        else
        {
            const_cast<PropertyInfo&>(prop).Get<std::shared_ptr<TResource>>(instance) = resource;
        }
    }
}
