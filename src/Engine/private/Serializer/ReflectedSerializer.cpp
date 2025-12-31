#include "Serializer/ReflectedSerializer.h"
#include "Core/ClassInfo.h"
#include "Core/PropertyInfo.h"
#include "Debug/Logger.h"
#include "Math/Vector3.h"
#include "Math/Vector2.h"
#include "Math/Rect.h"
#include <SDL3/SDL_pixels.h>
#include "Containers/SubclassOf.h"
#include "Utils/ReflectionUtils.h"

#include <unordered_map>
#include <functional>

#include "Ressources/RessourcesClass/AudioClip.h"
#include "Ressources/RessourcesClass/AudioContainer.h"
#include "Ressources/RessourcesClass/SpriteAtlas.h"
#include "Ressources/RessourcesClass/Texture.h"

namespace BixEngine::Serialization
{
    using namespace Bix::Reflection;

    // --- TypeHandler Structure ---
    struct TypeHandler
    {
        std::function<bool(const PropertyInfo&, const void*, Utils::BinaryWriter&)> Serialize;
        std::function<bool(const PropertyInfo&, void*, Utils::BinaryReader&)> Deserialize;
        std::function<bool(Utils::BinaryReader&)> Skip;
    };

    // --- Handler Registry ---
    static const std::unordered_map<std::string, TypeHandler>& GetHandlers()
    {
        static std::unordered_map<std::string, TypeHandler> handlers;
        if (handlers.empty())
        {
            #define REGISTER_PRIMITIVE(Type, Name) \
            { \
                TypeHandler h; \
                h.Serialize = [](const PropertyInfo& p, const void* i, Utils::BinaryWriter& w) -> bool { return w.WritePrimitive(p.Get<Type>(i)); }; \
                h.Deserialize = [](const PropertyInfo& p, void* i, Utils::BinaryReader& r) -> bool { Type v; if(!r.ReadPrimitive(v)) return false; const_cast<PropertyInfo&>(p).Get<Type>(i) = v; return true; }; \
                h.Skip = [](Utils::BinaryReader& r) -> bool { Type v; return r.ReadPrimitive(v); }; \
                handlers[Name] = h; \
            }

            REGISTER_PRIMITIVE(int, "int")
            REGISTER_PRIMITIVE(int, "int32_t")
            REGISTER_PRIMITIVE(float, "float")
            REGISTER_PRIMITIVE(bool, "bool")
            REGISTER_PRIMITIVE(SDL_Color, "SDL_Color")

            // --- Strings ---
            TypeHandler stringHandler;
            stringHandler.Serialize = [](const PropertyInfo& p, const void* i, Utils::BinaryWriter& w) -> bool
            { 
                return w.WriteString(p.Get<String>(i)); 
            };
            
            stringHandler.Deserialize = [](const PropertyInfo& p, void* i, Utils::BinaryReader& r) -> bool
            { 
                String v; if(!r.ReadString(v))
                    return false;
                
                p.Get<String>(i) = v;
                
                return true; 
            };
            
            stringHandler.Skip = [](Utils::BinaryReader& r) -> bool
            { 
                String v;
                return r.ReadString(v); 
            };
            
            handlers["String"] = stringHandler;
            handlers["std::string"] = stringHandler; 

            // --- Math::Vector2<T> ---
            auto RegisterVec2 = [&]<typename T>(const std::string& name)
            {
                TypeHandler h;
                h.Serialize = [](const PropertyInfo& p, const void* i, Utils::BinaryWriter& w) -> bool { 
                    const auto& v = p.Get<Math::TVector2<T>>(i); 
                    return w.WritePrimitive(v.x) && w.WritePrimitive(v.y); 
                };
                h.Deserialize = [](const PropertyInfo& p, void* i, Utils::BinaryReader& r) -> bool { 
                    Math::TVector2<T> v; 
                    if(!r.ReadPrimitive(v.x) || !r.ReadPrimitive(v.y)) return false;
                    const_cast<PropertyInfo&>(p).Get<Math::TVector2<T>>(i) = v; 
                    return true; 
                };
                h.Skip = [](Utils::BinaryReader& r) -> bool { 
                    T temp; return r.ReadPrimitive(temp) && r.ReadPrimitive(temp); 
                };
                handlers[name] = h;
            };

            // Math::Vector3<T>
            auto RegisterVec3 = [&]<typename T>(const std::string& name)
            {
                TypeHandler h;
                h.Serialize = [](const PropertyInfo& p, const void* i, Utils::BinaryWriter& w) -> bool
                { 
                    const auto& v = p.Get<Math::TVector3<T>>(i); 
                    return w.WritePrimitive(v.x) && w.WritePrimitive(v.y) && w.WritePrimitive(v.z); 
                };
                h.Deserialize = [](const PropertyInfo& p, void* i, Utils::BinaryReader& r) -> bool
                { 
                    Math::TVector3<T> v; 
                    if(!r.ReadPrimitive(v.x) || !r.ReadPrimitive(v.y) || !r.ReadPrimitive(v.z))
                        return false;
                    
                    const_cast<PropertyInfo&>(p).Get<Math::TVector3<T>>(i) = v; 
                    return true; 
                };
                h.Skip = [](Utils::BinaryReader& r) -> bool
                { 
                    T temp;
                    return r.ReadPrimitive(temp) && r.ReadPrimitive(temp) && r.ReadPrimitive(temp); 
                };
                handlers[name] = h;
            };

            // --- Enregistrement des types ---

            // Vector2 Float
            RegisterVec2.operator()<float>("Math::Vector2");
            RegisterVec2.operator()<float>("Math::Vector2<float>");

            // Vector2 Int
            RegisterVec2.operator()<int>("Math::Vector2<int>");
            RegisterVec2.operator()<int>("Math::Vec2i");

            // Vector3 Float
            RegisterVec3.operator()<float>("Math::Vector3");
            RegisterVec3.operator()<float>("Math::Vector3<float>");

            // Vector3 Int
            RegisterVec3.operator()<int>("Math::Vector3<int>");
            RegisterVec3.operator()<int>("Math::Vec3i");

            // TArray<T>
            auto RegisterArray = [&]<typename T>(const std::string& name)
            {
                TypeHandler h;
                
                h.Serialize = [](const PropertyInfo& p, const void* i, Utils::BinaryWriter& w) -> bool
                { 
                    const auto& arr = p.Get<TArray<T>>(i);
                    if (!w.WriteUint32(static_cast<uint32_t>(arr.size())))
                        return false;
                    
                    for (const auto& val : arr)
                    {
                        if (!w.WritePrimitive(val))
                            return false;
                    }
                    
                    return true;
                };

                h.Deserialize = [](const PropertyInfo& p, void* i, Utils::BinaryReader& r) -> bool
                { 
                    TArray<T>& arr = const_cast<PropertyInfo&>(p).Get<TArray<T>>(i);
                    uint32_t count = 0;
                    if (!r.ReadUint32(count))
                        return false;
                    
                    arr.resize(count);
                    for (uint32_t k = 0; k < count; ++k)
                    {
                        if (!r.ReadPrimitive(arr[k]))
                            return false;
                    }
                    
                    return true; 
                };

                h.Skip = [](Utils::BinaryReader& r) -> bool
                { 
                    uint32_t count = 0;
                    if (!r.ReadUint32(count))
                        return false;
                    
                    for (uint32_t k = 0; k < count; ++k)
                    {
                        T temp; 
                        if (!r.ReadPrimitive(temp))
                            return false;
                    }
                    
                    return true;
                };

                handlers[name] = h;
            };

            // --- Enregistrement des Tableaux ---
            RegisterArray.operator()<int>("TArray<int>");
            RegisterArray.operator()<float>("TArray<float>");
            RegisterArray.operator()<bool>("TArray<bool>");
            RegisterArray.operator()<Math::Vector3>("TArray<Math::Vector3>");

            // --- Math::Rect ---
            TypeHandler rectHandler;
            rectHandler.Serialize = [](const PropertyInfo& p, const void* i, Utils::BinaryWriter& w) -> bool
            { 
                const auto& v = p.Get<Math::Rect>(i); 
                return w.WritePrimitive(v.x) && w.WritePrimitive(v.y) && w.WritePrimitive(v.width) && w.WritePrimitive(v.height);
            };
            
            rectHandler.Deserialize = [](const PropertyInfo& p, void* i, Utils::BinaryReader& r) -> bool
            { 
                Math::Rect v; 
                if(!r.ReadPrimitive(v.x) || !r.ReadPrimitive(v.y) || !r.ReadPrimitive(v.width) || !r.ReadPrimitive(v.height))
                    return false;
                
                p.Get<Math::Rect>(i) = v; 
                return true; 
            };
            
            rectHandler.Skip = [](Utils::BinaryReader& r) -> bool
            {
                float f;
                return r.ReadPrimitive(f) && r.ReadPrimitive(f) && r.ReadPrimitive(f) && r.ReadPrimitive(f);
            };

            handlers["Math::Rect"] = rectHandler;
        }
        return handlers;
    }

    // --- Skip Logic ---
    static bool SkipValue(Utils::BinaryReader& reader, const std::string& type)
    {
        // 1. Try Map
        const auto& handlers = GetHandlers();
        if (auto it = handlers.find(type); it != handlers.end())
        {
            return it->second.Skip(reader);
        }

        // 2. Fallbacks
        // Resources -> String
        if (Utils::IsResourceType(type, "Texture") ||
            Utils::IsResourceType(type, "AudioClip") || 
            Utils::IsResourceType(type, "SpriteAtlas") ||
            Utils::IsResourceType(type, "AudioContainer"))
        {
            String v; return reader.ReadString(v);
        }

        // TSubclassOf -> String
        if (type.find("TSubclassOf<") != std::string::npos && type.find("TArray") == std::string::npos)
        {
            String v; return reader.ReadString(v);
        }

        // TArray<T> -> Read N Strings
        if (type.find("TArray<") != std::string::npos)
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

        return false;
    }

    // --- ReflectedSerializer Implementation ---

    bool ReflectedSerializer::Serialize(const void* instance, const ClassInfo* info, Utils::BinaryWriter& writer, int depth)
    {
        if (!instance || !info)
            return false;
        
        if (depth > 20)
            return false;

        // 1. Parent
        if (info->SuperClass && !Serialize(instance, info->SuperClass, writer, depth + 1)) 
            return false;

        const auto& handlers = GetHandlers();

        // 2. Count Valid Properties
        uint32_t validPropCount = 0;
        for (const auto& prop : info->Properties)
        {
            const std::string& type = prop.TypeName;
            if (handlers.contains(type) || 
                Utils::IsResourceType(type, "Texture") ||
                Utils::IsResourceType(type, "AudioClip") || 
                Utils::IsResourceType(type, "SpriteAtlas") ||
                Utils::IsResourceType(type, "AudioContainer") ||
                type.find("TSubclassOf<") != std::string::npos || prop.ArrayFunctions != nullptr)
            {
                validPropCount++;
            }
        }

        if (!writer.WriteUint32(validPropCount))
            return false;

        // 3. Serialize Properties
        for (const auto& prop : info->Properties)
        {
            const std::string& type = prop.TypeName;

            // Map Handler
            if (auto it = handlers.find(type); it != handlers.end())
            {
                if (!writer.WriteString(prop.Name))
                    return false;
                
                if (!writer.WriteString(prop.TypeName))
                    return false;
                
                if (!it->second.Serialize(prop, instance, writer))
                    return false;
                
                continue;
            }

            // Fallback: Resources
            if (Utils::IsResourceType(type, "Texture") ||
                Utils::IsResourceType(type, "AudioClip") || 
                Utils::IsResourceType(type, "SpriteAtlas") ||
                Utils::IsResourceType(type, "AudioContainer"))
            {
                if (!writer.WriteString(prop.Name))
                    return false;
                
                if (!writer.WriteString(prop.TypeName))
                    return false;

                String path = "";
                if (Utils::IsResourceType(type, "Texture"))
                {
                    path = Utils::GetResourcePath<Resources::Texture>(prop, instance);
                }
                else if (Utils::IsResourceType(type, "AudioClip"))
                {
                    path = Utils::GetResourcePath<Resources::AudioClip>(prop, instance);
                }
                else if (Utils::IsResourceType(type, "SpriteAtlas"))
                {
                    path = Utils::GetResourcePath<Resources::SpriteAtlas>(prop, instance);
                }
                else if (Utils::IsResourceType(type, "AudioContainer"))
                {
                    path = Utils::GetResourcePath<Resources::AudioContainer>(prop, instance);
                }
                
                if (!writer.WriteString(path))
                    return false;
                
                continue;
            }

            // Fallback: TArray
            if (prop.ArrayFunctions)
            {
                if (!writer.WriteString(prop.Name))
                    return false;
                
                if (!writer.WriteString(prop.TypeName))
                    return false;

                std::size_t size = prop.ArrayFunctions->GetSize(instance);
                if (!writer.WriteUint32(static_cast<uint32_t>(size)))
                    return false;
                
                for (std::size_t i = 0; i < size; ++i)
                {
                    std::string path = prop.ArrayFunctions->GetStringAt(instance, i);
                    if (!writer.WriteString(String(path.c_str())))
                        return false;
                }
                
                continue;
            }

            // Fallback: TSubclassOf
            if (type.find("TSubclassOf<") != std::string::npos)
            {
                if (!writer.WriteString(prop.Name))
                    return false;
                
                if (!writer.WriteString(prop.TypeName))
                    return false;

                const auto& subclass = prop.Get<TSubclassOf<Utils::GenericSubclassType>>(instance);
                if (!writer.WriteString(subclass.GetAssetPath()))
                    return false;
            }
        }
        
        return true;
    }

    bool ReflectedSerializer::Deserialize(void* instance, const ClassInfo* info, Utils::BinaryReader& reader, int depth)
    {
        if (!instance || !info) return false;
        if (depth > 20) return false;

        // 1. Parent
        if (info->SuperClass && !Deserialize(instance, info->SuperClass, reader, depth + 1)) 
            return false;

        const auto& handlers = GetHandlers();

        // 2. Count
        uint32_t propCount = 0;
        if (!reader.ReadUint32(propCount))
            return false;

        // 3. Properties
        for (uint32_t i = 0; i < propCount; ++i)
        {
            String propName;
            String typeName;
            
            if (!reader.ReadString(propName) || !reader.ReadString(typeName)) 
            {
                LOG_ERROR("ReflectedSerializer: Failed to read metadata at " + std::to_string(i));
                return false;
            }

            // Find Property
            const PropertyInfo* prop = nullptr;
            for (const auto& p : info->Properties)
            {
                if (p.Name == propName.Std())
                {
                    prop = &p;
                    break;
                }
            }

            if (!prop)
            {
                if (!SkipValue(reader, typeName.Std()))
                    return false;
                
                continue;
            }

            if (prop->TypeName != typeName.Std())
            {
                LOG_WARNING("ReflectedSerializer: Type mismatch for " + propName.Std() + ". Skipping.");
                if (!SkipValue(reader, typeName.Std()))
                    return false;
                
                continue;
            }

            const std::string& type = prop->TypeName;

            // Map Handler
            if (auto it = handlers.find(type); it != handlers.end())
            {
                if (!it->second.Deserialize(*prop, instance, reader))
                    return false;
                
                continue;
            }

            // Fallback: Resources
            if (Utils::IsResourceType(type, "Texture") || Utils::IsResourceType(type, "AudioClip") || 
                Utils::IsResourceType(type, "SpriteAtlas") || Utils::IsResourceType(type, "AudioContainer"))
            {
                String path;
                if (!reader.ReadString(path))
                    return false;
                
                if (!path.empty())
                {
                     if (Utils::IsResourceType(type, "Texture"))
                     {
                         Utils::SetResourceFromPath<Resources::Texture>(*prop, instance, path);
                     }
                     else if (Utils::IsResourceType(type, "AudioClip"))
                     {
                         Utils::SetResourceFromPath<Resources::AudioClip>(*prop, instance, path);
                     }
                     else if (Utils::IsResourceType(type, "SpriteAtlas"))
                     {
                         Utils::SetResourceFromPath<Resources::SpriteAtlas>(*prop, instance, path);
                     }
                     else if (Utils::IsResourceType(type, "AudioContainer"))
                     {
                         Utils::SetResourceFromPath<Resources::AudioContainer>(*prop, instance, path);
                     }
                }
                
                continue;
            }

            // Fallback: TArray
            if (prop->ArrayFunctions)
            {
                uint32_t count = 0;
                if (!reader.ReadUint32(count))
                    return false;
                
                prop->ArrayFunctions->Clear(instance);
                for (uint32_t k = 0; k < count; ++k)
                {
                    String path;
                    if (!reader.ReadString(path))
                        return false;
                    
                    prop->ArrayFunctions->AddString(instance, path.Std());
                }
                
                continue;
            }

            // Fallback: TSubclassOf
            if (type.find("TSubclassOf<") != std::string::npos)
            {
                String path;
                if (!reader.ReadString(path))
                    return false;
                
                auto& subclassObj = prop->Get<TSubclassOf<Utils::GenericSubclassType>>(instance);
                const_cast<String&>(subclassObj.GetAssetPath()) = path;
                continue;
            }

            LOG_ERROR("ReflectedSerializer: No handler for property " + propName.Std() + " (" + type + ")");
            return false;
        }

        return true;
    }
}