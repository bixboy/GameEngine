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
    bool ReflectedSerializer::Serialize(const void* instance, const ClassInfo* info, Utils::BinaryWriter& writer, int depth)
    {
        // LOG_INFO("ReflectedSerializer::Serialize: " + info->Name + " Depth: " + std::to_string(depth));
        if (!instance || !info) return false;

        // Prevent infinite recursion
        if (depth > 20)
        {
            LOG_ERROR("ReflectedSerializer::Serialize: Maximum recursion depth reached for type: " + info->Name);
            return false;
        }

        // 1. Parent Class
        if (info->SuperClass && !Serialize(instance, info->SuperClass, writer, depth + 1)) 
            return false;

        // 2. Properties
        for (const auto& prop : info->Properties)
        {
            const std::string& type = prop.TypeName;

            // Macro pour éviter la répétition des types primitifs
            #define SERIALIZE_PRIMITIVE(T, TypeString) \
                if (type == TypeString) { if (!writer.WritePrimitive(prop.Get<T>(instance))) return false; continue; }

            // --- Primitives ---
            SERIALIZE_PRIMITIVE(int, "int");
            SERIALIZE_PRIMITIVE(int, "int32_t");
            SERIALIZE_PRIMITIVE(float, "float");
            SERIALIZE_PRIMITIVE(bool, "bool");
            SERIALIZE_PRIMITIVE(SDL_Color, "SDL_Color");

            // --- Strings ---
            if (type == "String" || type == "std::string")
            {
                if (!writer.WriteString(prop.Get<String>(instance)))
                    return false;
                
                continue;
            }

            // --- Maths ---
            if (type.find("Vector3") != std::string::npos)
            {
                const auto& v = prop.Get<Math::Vector3>(instance);
                if (!writer.WritePrimitive(v.x) || !writer.WritePrimitive(v.y) || !writer.WritePrimitive(v.z))
                    return false;
                
                continue;
            }
            if (type.find("Vector2") != std::string::npos)
            {
                const auto& v = prop.Get<Math::Vector2<float>>(instance);
                if (!writer.WritePrimitive(v.x) || !writer.WritePrimitive(v.y))
                    return false;
                
                continue;
            }
            if (type.find("Rect") != std::string::npos)
            {
                const auto& r = prop.Get<Math::Rect>(instance);
                if (!writer.WritePrimitive(r.X) || !writer.WritePrimitive(r.Y) || !writer.WritePrimitive(r.Width) || !writer.WritePrimitive(r.Height))
                    return false;
                
                continue;
            }


            // --- Ressources (Texture, Audio, Atlas, AudioContainer) ---
            String path = "";
            if (IsResourceType(type, "Texture"))
                path = GetResourcePath<Texture>(prop, instance);
            
            else if (IsResourceType(type, "AudioClip"))
                path = GetResourcePath<AudioClip>(prop, instance);
            
            else if (IsResourceType(type, "SpriteAtlas"))
                path = GetResourcePath<SpriteAtlas>(prop, instance);

            else if (IsResourceType(type, "AudioContainer"))
                path = GetResourcePath<AudioContainer>(prop, instance);
            
            if (IsResourceType(type, "Texture") || IsResourceType(type, "AudioClip") || IsResourceType(type, "SpriteAtlas") || IsResourceType(type, "AudioContainer"))
            {
                if (!writer.WriteString(path))
                    return false;
                
                continue;
            }

            LOG_WARNING("ReflectedSerializer: Unsupported type '" + type + "' for property '" + prop.Name + "'");
        }
        
        #undef SERIALIZE_PRIMITIVE
        return true;
    }

    // ==============================================================================
    // DESERIALIZE
    // ==============================================================================
    bool ReflectedSerializer::Deserialize(void* instance, const ClassInfo* info, Utils::BinaryReader& reader, int depth)
    {
        if (!instance || !info)
            return false;

        // Prevent infinite recursion
        if (depth > 20)
        {
            LOG_ERROR("ReflectedSerializer::Deserialize: Maximum recursion depth reached for type: " + info->Name);
            return false;
        }

        if (info->SuperClass && !Deserialize(instance, info->SuperClass, reader, depth + 1)) 
            return false;

        for (const auto& prop : info->Properties)
        {
            const std::string& type = prop.TypeName;

            #define DESERIALIZE_PRIMITIVE(T, TypeString) \
                if (type == TypeString) { \
                    T val{}; if (!reader.ReadPrimitive(val)) return false; \
                    const_cast<PropertyInfo&>(prop).Get<T>(instance) = val; \
                    continue; \
                }

            // --- Primitives ---
            DESERIALIZE_PRIMITIVE(int, "int");
            DESERIALIZE_PRIMITIVE(int, "int32_t");
            DESERIALIZE_PRIMITIVE(float, "float");
            DESERIALIZE_PRIMITIVE(bool, "bool");
            DESERIALIZE_PRIMITIVE(SDL_Color, "SDL_Color");

            // --- Strings ---
            if (type == "String" || type == "std::string")
            {
                String val; 
                if (!reader.ReadString(val))
                    return false;
                
                prop.Get<String>(instance) = val;
                continue;
            }

            // --- Maths ---
            if (type.find("Vector3") != std::string::npos)
            {
                Math::Vector3 v;
                if (!reader.ReadPrimitive(v.x) || !reader.ReadPrimitive(v.y) || !reader.ReadPrimitive(v.z))
                    return false;
                
                prop.Get<Math::Vector3>(instance) = v;
                continue;
            }
            if (type.find("Vector2") != std::string::npos)
            {
                Math::Vector2<float> v;
                if (!reader.ReadPrimitive(v.x) || !reader.ReadPrimitive(v.y))
                    return false;
                
                prop.Get<Math::Vector2<float>>(instance) = v;
                continue;
            }
            if (type.find("Rect") != std::string::npos)
            {
                Math::Rect r;
                if (!reader.ReadPrimitive(r.X) || !reader.ReadPrimitive(r.Y) || !reader.ReadPrimitive(r.Width) || !reader.ReadPrimitive(r.Height))
                    return false;
                
                prop.Get<Math::Rect>(instance) = r;
                continue;
            }

            // --- Ressources ---
            if (IsResourceType(type, "Texture") || IsResourceType(type, "AudioClip") || IsResourceType(type, "SpriteAtlas") || IsResourceType(type, "AudioContainer"))
            {
                String path;
                if (!reader.ReadString(path)) return false;

                if (!path.IsEmpty())
                {
                    if (IsResourceType(type, "Texture"))
                        SetResourceFromPath<Texture>(prop, instance, path);
                    
                    else if (IsResourceType(type, "AudioClip"))
                        SetResourceFromPath<AudioClip>(prop, instance, path);
                    
                    else if (IsResourceType(type, "SpriteAtlas"))
                        SetResourceFromPath<SpriteAtlas>(prop, instance, path);

                    else if (IsResourceType(type, "AudioContainer"))
                        SetResourceFromPath<AudioContainer>(prop, instance, path);
                }
                else
                {
                    // Reset to null if path empty.
                }
            }
        }

        #undef DESERIALIZE_PRIMITIVE
        return true;
    }
}