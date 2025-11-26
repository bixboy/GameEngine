#include "Actor.h"
#include "Components/Component.h"
#include "Reflection/public/Registry.h"
#include "Utils/BinaryUtils.h"
#include "Logger.h"
#include <algorithm>
#include <memory>
#include <utility>
#include "Serializer/ReflectedSerializer.h"


namespace BixEngine::Game
{
    using namespace BixEngine::Utils;
    using namespace Bix::Reflection;
    
    namespace
    {
        const ClassInfo* FindClassInfo(const String& typeName)
        {
            auto* info = Registry::Get().Find(typeName.c_str());
            if (!info)
            {
                String qualifiedName = "BixEngine::Game::" + typeName;
                info = Registry::Get().FindByQualifiedName(qualifiedName.c_str());
                
                if (!info)
                {
                    info = Registry::Get().Find(qualifiedName.c_str());   
                }
            }
            
            return info;
        }

        Component* FindOrCreateComponent(Actor* actor, const String& typeName, size_t index)
        {
            auto& components = actor->GetComponents();

            if (index < components.size() && components[index]->GetTypeName() == typeName)
            {
                return components[index].get();
            }

            for (auto& comp : components)
            {
                if (comp->GetTypeName() == typeName)
                    return comp.get();
            }

            const auto* classInfo = FindClassInfo(typeName);
            if (classInfo && classInfo->CanConstruct())
            {
                if (auto* newComp = classInfo->ConstructTyped<Component>())
                {
                    actor->AddComponent(std::unique_ptr<Component>(newComp));
                    return actor->GetComponents().back().get();
                }
            }

            return nullptr;
        }
    }
    

    Actor::Actor(const Math::Transform& transform) : Object("Actor", transform)
    {}

    Actor::Actor(String name, const Math::Transform& transform) : Object(std::move(name), transform)
    {}

    void Actor::BeginPlay()
    {
        for (auto& c : components_)
            c->BeginPlay();
    }

    void Actor::Update(float deltaTime)
    {
        if (!active_)
            return;

        if (!has_begun_play_)
        {
            BeginPlay();
            has_begun_play_ = true;
        }

        for (auto& comp : components_)
            comp->Update(deltaTime);
    }

    void Actor::Render(Graphics::Renderer& renderer) const
    {
        if (!active_)
            return;
        
        for (const auto& comp : components_)
            comp->Render(renderer);
    }

    // ==============================================================================================
    // GESTION DES COMPOSANTS
    // ==============================================================================================

    void Actor::AddComponent(std::unique_ptr<Component> component)
    {
        if (component)
            components_.push_back(std::move(component));
    }

    bool Actor::RemoveComponent(const Component* component)
    {
        if (!component)
            return false;

        auto it = std::find_if(components_.begin(), components_.end(),
        [component](const auto& ptr)
        {
            return ptr.get() == component;
        });

        if (it == components_.end())
            return false;

        OnComponentRemoved(*(*it));
        components_.erase(it);
        return true;
    }

    std::unique_ptr<Actor> Actor::ClonePrototype() const
    {
        return std::make_unique<Actor>();
    }

    // ==============================================================================================
    // SÉRIALISATION
    // ==============================================================================================

    void Actor::SerializeBinary(std::ostream& stream) const
    {
        // 1. Base Object (Transform, Nom...)
        Object::SerializeBinary(stream);
        BinaryWriter writer(stream);

        // 2. Propriétés de l'Actor
        if (const auto* info = FindClassInfo(GetTypeName()))
        {
            Serialization::ReflectedSerializer::Serialize(this, info, writer);
        }
        else
        {
            LOG_WARNING("Serialize: No reflection info for Actor: " + GetTypeName());
        }

        // 3. Composants
        writer.WriteUint32(static_cast<uint32_t>(components_.size()));

        for (const auto& comp : components_)
        {
            String typeName = comp->GetTypeName();
            writer.WriteString(typeName);

            if (const auto* compInfo = FindClassInfo(typeName))
            {
                Serialization::ReflectedSerializer::Serialize(comp.get(), compInfo, writer);
            }
            else
            {
                LOG_WARNING("Serialize: No reflection info for Component: " + typeName);
            }
        }
    }

    // ==============================================================================================
    // DÉSÉRIALISATION
    // ==============================================================================================

    void Actor::DeserializeBinary(std::istream& stream)
    {
        // 1. Base Object
        Object::DeserializeBinary(stream);
        BinaryReader reader(stream);

        // 2. Propriétés de l'Actor
        if (const auto* info = FindClassInfo(GetTypeName()))
        {
            Serialization::ReflectedSerializer::Deserialize(this, info, reader);
        }

        // 3. Composants
        uint32_t compCount = 0;
        if (!reader.ReadUint32(compCount)) return;

        for (uint32_t i = 0; i < compCount; ++i)
        {
            String typeName;
            if (!reader.ReadString(typeName)) break;

            if (Component* comp = FindOrCreateComponent(this, typeName, i))
            {
                if (const auto* compInfo = FindClassInfo(typeName))
                {
                    Serialization::ReflectedSerializer::Deserialize(comp, compInfo, reader);
                }
            }
            else
            {
                LOG_ERROR("Deserialize: Failed to restore component of type: " + typeName);
            }
        }
    }
}
