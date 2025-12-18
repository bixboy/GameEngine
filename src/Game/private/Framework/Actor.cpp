#include "Framework/Actor.h"
#include "Components/Core/Component.h"
#include "Core/Registry.h"
#include "Utils/FileIO/BinaryUtils.h"
#include "Debug/Logger.h"
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
            if (info) return info;

            // Try common namespaces
            const std::array<String, 7> namespaces = {
                "BixEngine::Game::",
                "BixEngine::Render::",
                "BixEngine::Physics::",
                "BixEngine::Audio::",
                "BixEngine::Core::",
                "BixEngine::Systems::",
                "BixEngine::Gui::"
            };

            for (const auto& ns : namespaces)
            {
                String qualifiedName = ns + typeName;
                if (auto* nsInfo = Registry::Get().FindByQualifiedName(qualifiedName.c_str()))
                    return nsInfo;
                
                // Fallback to simple Find if qualified name wasn't registered as such but map key matches?
                // Registry usually keys by what BCLASS registers.
            }
            
            return nullptr;
        }

        Component* FindOrCreateComponent(Actor* actor, const String& typeName, size_t index)
        {
            auto& components = actor->GetComponents();

            // 1. Direct match at expected index (fast path)
            if (index < components.size() && components[index]->GetTypeName() == typeName)
            {
                return components[index].get();
            }

            // 2. Loose match: search forward to find duplicates
            for (size_t i = index; i < components.size(); ++i)
            {
                if (components[i]->GetTypeName() == typeName)
                    return components[i].get();
            }

            // 3. Create new component via Reflection
            const auto* classInfo = FindClassInfo(typeName);
            if (classInfo && classInfo->CanConstruct())
            {
                if (auto* newComp = classInfo->ConstructTyped<Component>())
                {
                    actor->AddComponent(std::unique_ptr<Component>(newComp));
                    return actor->GetComponents().back().get();
                }
                else
                {
                    LOG_ERROR("FindOrCreateComponent: ConstructTyped returned nullptr for '" + typeName + "'");
                }
            }
            else
            {
                LOG_ERROR("FindOrCreateComponent: Could not create component '" + typeName + "'. Missing ClassInfo or Reflection data.");
            }

            return nullptr;
        }
    }
    

    Actor::Actor(const Math::Transform& transform) : Object("Actor", transform)
    {}

    Actor::Actor(String name, const Math::Transform& transform) : Object(std::move(name), transform)
    {}

    Actor::~Actor()
    {
        // Unlink from parent
        SetParent(nullptr);

        // Unlink children (they become orphans, but usually Scene::RemoveActor handles their deletion)
        // We copy the list because SetParent modifies it
        auto kids = children_;
        for (auto* child : kids)
        {
            if (child) child->SetParent(nullptr);
        }
    }

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
        {
            component->SetOwner(this);
            components_.push_back(std::move(component));
        }
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

    // ==============================================================================================
    // HIERARCHY
    // ==============================================================================================

    void Actor::SetParent(Actor* parent)
    {
        if (parent_ == parent)
            return;

        if (parent == this)
        {
            LOG_WARNING("Cannot set parent to self: " + GetName());
            return;
        }

        if (parent && parent->IsChildOf(this))
        {
            LOG_WARNING("Cannot set parent to a child (cycle detected): " + GetName());
            return;
        }

        // 1. Remove from old parent
        if (parent_)
        {
            auto& siblings = parent_->children_;
            std::erase(siblings, this);
        }

        // 2. Set new parent
        parent_ = parent;

        // 3. Add to new parent & Link Transform
        if (parent_)
        {
            parent_->children_.push_back(this);
            GetTransformRef().SetParent(&parent_->GetTransformRef());
        }
        else
        {
            GetTransformRef().SetParent(nullptr);
        }
    }

    bool Actor::IsChildOf(const Actor* potentialParent) const
    {
        if (!potentialParent)
            return false;

        const Actor* current = parent_;
        while (current)
        {
            if (current == potentialParent)
                return true;
            current = current->parent_;
        }
        return false;
    }

    // ==============================================================================================
    // CLONING
    // ==============================================================================================

    std::unique_ptr<Actor> Actor::ClonePrototype() const
    {
        return std::make_unique<Actor>();
        // Note: Hierarchy is not cloned for prototypes by default
    }

    // ==============================================================================================
    // SÉRIALISATION
    // ==============================================================================================

    void Actor::SerializeBinary(std::ostream& stream) const
    {
        // 1. Base Object (Transform, Nom...)
        Object::SerializeBinary(stream);
        BinaryWriter writer(stream);

        // Hierarchy
        String psUUID = parent_ ? parent_->GetUUID() : String();
        writer.WriteString(psUUID);

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

        // Hierarchy
        reader.ReadString(parentUUID_);

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
