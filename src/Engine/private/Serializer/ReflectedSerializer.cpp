#include "Serializer/ReflectedSerializer.h"
#include "Core/ClassInfo.h"
#include "Core/PropertyInfo.h"
#include "Debug/Logger.h"
#include "Math/Vector3.h"
#include "Math/Vector2.h"
#include "Math/Rect.h"
#include <SDL3/SDL_pixels.h>
#include "Ressources/Core/ResourceManager.h"
#include "Ressources/RessourcesClass/AudioClip.h"
#include "Ressources/RessourcesClass/Texture.h"
#include "Ressources/RessourcesClass/SpriteAtlas.h"
#include "Ressources/RessourcesClass/AudioContainer.h"


#include "Templates/SubclassOf.h"
#include "Containers/Array.h"

namespace BixEngine::Serialization
{
    using namespace Bix::Reflection;
    using namespace BixEngine::resources;

    static bool IsResourceType(const std::string& typeName, const std::string& resourceName)
    {
        return typeName.find(resourceName) != std::string::npos;
    }

    template <typename TResource>
    static String GetResourcePath(const PropertyInfo& prop, const void* instance)
    {
        // std::shared_ptr<T>
        if (prop.TypeName.find("shared_ptr") != std::string::npos)
        {
            const auto& ptr = prop.Get<std::shared_ptr<TResource>>(instance);
            return ptr ? ptr->GetPath() : String("");
        }
        // T* (Raw Pointer)
        if (prop.TypeName.find("*") != std::string::npos)
        {
            const auto* ptr = prop.Get<TResource*>(instance);
            return ptr ? ptr->GetPath() : String("");
        }
        return "";
    }

    template <typename TResource>
    static void SetResourceFromPath(const PropertyInfo& prop, void* instance, const String& path)
    {
        auto resource = ResourceManager::Get().Get<TResource>(path);

        if (prop.TypeName.find("shared_ptr") != std::string::npos)
        {
            // const_cast nécessaire car Get() retourne const& par défaut sur PropertyInfo const
            const_cast<PropertyInfo&>(prop).Get<std::shared_ptr<TResource>>(instance) = resource;
        }
        else if (prop.TypeName.find("*") != std::string::npos)
        {
            const_cast<PropertyInfo&>(prop).Get<TResource*>(instance) = resource.get();
        }
    }

    // ==============================================================================
    // SERIALIZE
    // ==============================================================================
    // Helper to skip values of unknown properties based on their type
    static bool SkipValue(Utils::BinaryReader& reader, const std::string& type)
    {
        // Primitives
        if (type.compare("int") == 0 || type.compare("int32_t") == 0) { int v; return reader.ReadPrimitive(v); }
        if (type.compare("float") == 0) { float v; return reader.ReadPrimitive(v); }
        if (type.compare("bool") == 0) { bool v; return reader.ReadPrimitive(v); }
        if (type.compare("SDL_Color") == 0) { SDL_Color v; return reader.ReadPrimitive(v); }
        
        // Strings & Resources (Saved as Path String)
        if (type.compare("String") == 0 || type.compare("std::string") == 0 || 
            IsResourceType(type, "Texture") || IsResourceType(type, "AudioClip") || 
            IsResourceType(type, "SpriteAtlas") || IsResourceType(type, "AudioContainer"))
        {
            String v; return reader.ReadString(v);
        }

        // TArray<TSubclassOf<T>>
        if (type.find("TArray<TSubclassOf<") != std::string::npos)
        {
            uint32_t count = 0;
            if (!reader.ReadUint32(count)) return false;
            for (uint32_t i = 0; i < count; ++i)
            {
                String v; 
                if(!reader.ReadString(v)) return false;
            }
            return true;
        }

        // TSubclassOf<T> - Treated as String
        if (type.find("TSubclassOf<") != std::string::npos)
        {
            String v; return reader.ReadString(v);
        }

        // Math
        if (type.find("Vector3") != std::string::npos) { Math::Vector3 v; return reader.ReadPrimitive(v.x) && reader.ReadPrimitive(v.y) && reader.ReadPrimitive(v.z); }
        if (type.find("Vector2") != std::string::npos) { Math::Vector2<float> v; return reader.ReadPrimitive(v.x) && reader.ReadPrimitive(v.y); }
        if (type.find("Rect") != std::string::npos) { Math::Rect v; return reader.ReadPrimitive(v.X) && reader.ReadPrimitive(v.Y) && reader.ReadPrimitive(v.Width) && reader.ReadPrimitive(v.Height); }

        // Pointers (e.g. Actor*, Component*) 
        return false;
    }

    // ==============================================================================
    // SERIALIZE
    // ==============================================================================
    bool ReflectedSerializer::Serialize(const void* instance, const ClassInfo* info, Utils::BinaryWriter& writer, int depth)
    {
        if (!instance || !info) return false;
        if (depth > 20) return false;

        // 1. Parent Class
        if (info->SuperClass && !Serialize(instance, info->SuperClass, writer, depth + 1)) 
            return false;

        // 2. Count Valid Properties FIRST to match header
        uint32_t validPropCount = 0;
        for (const auto& prop : info->Properties)
        {
            const std::string& type = prop.TypeName;
            bool isSupported = 
                type == "int" || type == "int32_t" || type == "float" || type == "bool" || type == "SDL_Color" ||
                type == "String" || type == "std::string" ||
                type.find("Vector3") != std::string::npos || type.find("Vector2") != std::string::npos || type.find("Rect") != std::string::npos ||
                IsResourceType(type, "Texture") || IsResourceType(type, "AudioClip") || IsResourceType(type, "SpriteAtlas") || IsResourceType(type, "AudioContainer") ||
                type.find("TSubclassOf<") != std::string::npos ||
                (prop.ArrayFunctions != nullptr);

            if (isSupported)
                validPropCount++;
        }

        if (!writer.WriteUint32(validPropCount)) return false;

        // 3. Properties Data
        for (const auto& prop : info->Properties)
        {
            const std::string& type = prop.TypeName;
            
            bool isSupported = 
                type == "int" || type == "int32_t" || type == "float" || type == "bool" || type == "SDL_Color" ||
                type == "String" || type == "std::string" ||
                type.find("Vector3") != std::string::npos || type.find("Vector2") != std::string::npos || type.find("Rect") != std::string::npos ||
                IsResourceType(type, "Texture") || IsResourceType(type, "AudioClip") || IsResourceType(type, "SpriteAtlas") || IsResourceType(type, "AudioContainer") ||
                type.find("TSubclassOf<") != std::string::npos ||
                (prop.ArrayFunctions != nullptr);

            if (!isSupported)
            {
                LOG_WARNING("ReflectedSerializer: Skipping unsupported property '" + prop.Name + "' of type '" + type + "'");
                continue;
            }

            // Write Metadata
            if (!writer.WriteString(prop.Name)) return false;
            if (!writer.WriteString(prop.TypeName)) return false;

            // Write Value
            #define SERIALIZE_PRIMITIVE(T, TypeString) \
                if (type.compare(TypeString) == 0) { if (!writer.WritePrimitive(prop.Get<T>(instance))) return false; continue; }

            SERIALIZE_PRIMITIVE(int, "int");
            SERIALIZE_PRIMITIVE(int, "int32_t");
            SERIALIZE_PRIMITIVE(float, "float");
            SERIALIZE_PRIMITIVE(bool, "bool");
            SERIALIZE_PRIMITIVE(SDL_Color, "SDL_Color");

            if (type.compare("String") == 0 || type.compare("std::string") == 0)
            {
                if (!writer.WriteString(prop.Get<String>(instance))) return false;
                continue;
            }

            if (type.find("Vector3") != std::string::npos)
            {
                const auto& v = prop.Get<Math::Vector3>(instance);
                if (!writer.WritePrimitive(v.x) || !writer.WritePrimitive(v.y) || !writer.WritePrimitive(v.z)) return false;
                continue;
            }
            if (type.find("Vector2") != std::string::npos)
            {
                const auto& v = prop.Get<Math::Vector2<float>>(instance);
                if (!writer.WritePrimitive(v.x) || !writer.WritePrimitive(v.y)) return false;
                continue;
            }
            if (type.find("Rect") != std::string::npos)
            {
                const auto& r = prop.Get<Math::Rect>(instance);
                if (!writer.WritePrimitive(r.X) || !writer.WritePrimitive(r.Y) || !writer.WritePrimitive(r.Width) || !writer.WritePrimitive(r.Height)) return false;
                continue;
            }

            // Resources
            if (IsResourceType(type, "Texture") || IsResourceType(type, "AudioClip") || IsResourceType(type, "SpriteAtlas") || IsResourceType(type, "AudioContainer"))
            {
                String path = "";
                if (IsResourceType(type, "Texture")) path = GetResourcePath<Texture>(prop, instance);
                else if (IsResourceType(type, "AudioClip")) path = GetResourcePath<AudioClip>(prop, instance);
                else if (IsResourceType(type, "SpriteAtlas")) 
                {
                    path = GetResourcePath<SpriteAtlas>(prop, instance);
                }
                else if (IsResourceType(type, "AudioContainer")) path = GetResourcePath<AudioContainer>(prop, instance);
                
                if (!writer.WriteString(path)) return false;
                continue;
            }

            // TArray Support
            if (prop.ArrayFunctions)
            {
                std::size_t size = prop.ArrayFunctions->GetSize(instance);
                if (!writer.WriteUint32(static_cast<uint32_t>(size))) return false;
                
                for (std::size_t i = 0; i < size; ++i)
                {
                    std::string path = prop.ArrayFunctions->GetStringAt(instance, i);
                    if (!writer.WriteString(String(path.c_str()))) return false;
                }
                continue;
            }

            // TSubclassOf Support
            // Note: This must come AFTER TArray because TArray also contains "TSubclassOf", but TArray has ArrayFunctions set.
            // If it's a single TSubclassOf, ArrayFunctions is likely null.
            if (type.find("TSubclassOf<") != std::string::npos)
            {
                // Unsafe cast to generic TSubclassOf<int> to access the string path
                // This assumes standard layout where `path_` is the only member.
                const auto& subclass = prop.Get<TSubclassOf<int>>(instance);
                if (!writer.WriteString(subclass.GetAssetPath())) return false;
                continue;
            }

            return false;
        }
        
        #undef SERIALIZE_PRIMITIVE
        return true;
    }

    // ==============================================================================
    // DESERIALIZE
    // ==============================================================================
    bool ReflectedSerializer::Deserialize(void* instance, const ClassInfo* info, Utils::BinaryReader& reader, int depth)
    {
        if (!instance || !info) return false;
        if (depth > 20) return false;

        if (info->SuperClass && !Deserialize(instance, info->SuperClass, reader, depth + 1)) 
            return false;

        // 1. Read Property Count
        uint32_t propCount = 0;
        if (!reader.ReadUint32(propCount)) return false;

        // 3. Properties Data
        for (uint32_t i = 0; i < propCount; ++i)
        {
            String propName;
            String typeName;
            if (!reader.ReadString(propName) || !reader.ReadString(typeName)) 
            {
                LOG_ERROR("ReflectedSerializer: Failed to read Property Metadata at index " + std::to_string(i));
                return false;
            }

            const PropertyInfo* prop = nullptr;
            for (const auto& p : info->Properties)
            {
                if (p.Name == propName.ToStdString())
                {
                    prop = &p;
                    break;
                }
            }

            if (prop)
            {
                 // Type Safe Check?
                 if (prop->TypeName != typeName.ToStdString())
                 {
                     LOG_WARNING("ReflectedSerializer: Type Mismatch for '" + propName.ToStdString() + "'. File: " + typeName.ToStdString() + ", Code: " + prop->TypeName + ". Skipping.");
                     if (!SkipValue(reader, typeName.ToStdString())) return false;
                     continue;
                 }

                 const std::string& type = prop->TypeName;

                #define DESERIALIZE_PRIMITIVE(T, TypeString) \
                    if (type.compare(TypeString) == 0) { \
                        T val{}; if (!reader.ReadPrimitive(val)) return false; \
                        const_cast<PropertyInfo*>(prop)->Get<T>(instance) = val; \
                         goto continue_loop; \
                    }

                DESERIALIZE_PRIMITIVE(int, "int");
                DESERIALIZE_PRIMITIVE(int, "int32_t");
                DESERIALIZE_PRIMITIVE(float, "float");
                DESERIALIZE_PRIMITIVE(bool, "bool");
                DESERIALIZE_PRIMITIVE(SDL_Color, "SDL_Color");

                if (type.compare("String") == 0 || type.compare("std::string") == 0)
                {
                    String val; 
                    if (!reader.ReadString(val)) return false;
                    prop->Get<String>(instance) = val;
                    continue;
                }

                if (type.find("Vector3") != std::string::npos)
                {
                    Math::Vector3 v;
                    if (!reader.ReadPrimitive(v.x) || !reader.ReadPrimitive(v.y) || !reader.ReadPrimitive(v.z)) return false;
                    prop->Get<Math::Vector3>(instance) = v;
                    continue;
                }
                if (type.find("Vector2") != std::string::npos)
                {
                    Math::Vector2<float> v;
                    if (!reader.ReadPrimitive(v.x) || !reader.ReadPrimitive(v.y)) return false;
                    prop->Get<Math::Vector2<float>>(instance) = v;
                    continue;
                }
                if (type.find("Rect") != std::string::npos)
                {
                    Math::Rect r;
                    if (!reader.ReadPrimitive(r.X) || !reader.ReadPrimitive(r.Y) || !reader.ReadPrimitive(r.Width) || !reader.ReadPrimitive(r.Height)) return false;
                    prop->Get<Math::Rect>(instance) = r;
                    continue;
                }

                if (IsResourceType(type, "Texture") || IsResourceType(type, "AudioClip") || IsResourceType(type, "SpriteAtlas") || IsResourceType(type, "AudioContainer"))
                {
                    String path;
                    if (!reader.ReadString(path)) return false;
                    if (!path.IsEmpty())
                    {
                        if (IsResourceType(type, "Texture")) SetResourceFromPath<Texture>(*prop, instance, path);
                        else if (IsResourceType(type, "AudioClip")) SetResourceFromPath<AudioClip>(*prop, instance, path);
                        else if (IsResourceType(type, "SpriteAtlas")) SetResourceFromPath<SpriteAtlas>(*prop, instance, path);
                        else if (IsResourceType(type, "AudioContainer")) SetResourceFromPath<AudioContainer>(*prop, instance, path);
                    }
                    continue;
                }

                if (prop->ArrayFunctions)
                {
                    uint32_t count = 0;
                    if (!reader.ReadUint32(count)) return false;
                    prop->ArrayFunctions->Clear(instance);
                    for (uint32_t k = 0; k < count; ++k)
                    {
                        String path;
                        if (!reader.ReadString(path)) return false;
                        prop->ArrayFunctions->AddString(instance, path.ToStdString());
                    }
                    continue;
                }

                // TSubclassOf Support (Generic reading via int alias)
                if (type.find("TSubclassOf<") != std::string::npos)
                {
                    String path;
                    if (!reader.ReadString(path)) return false;
                    
                    // Unsafe cast to generic TSubclassOf<int>
                    const_cast<PropertyInfo*>(prop)->Get<TSubclassOf<int>>(instance).GetAssetPath() = path;
                    continue;
                }
                
                // Fallback catch-all
                 LOG_ERROR("ReflectedSerializer: Matched property '" + propName + "' type='" + type + "' but no logic to read it. Desync imminent.");
                 return false;

                continue_loop:; 
            }
            else
            {
                if (!SkipValue(reader, typeName.ToStdString())) return false;
            }
        }
        return true;
    }
}