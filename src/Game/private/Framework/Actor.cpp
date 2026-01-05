#include "Framework/Actor.h"
#include "Core/Registry.h"
#include "Utils/FileIO/BinaryUtils.h"
#include "Serializer/ReflectedSerializer.h"
#include "Debug/Logger.h"
#include <algorithm>

namespace BixEngine::Game
{
    using namespace BixEngine::Utils;
    using namespace Bix::Reflection;
    
    namespace
    {
        const ClassInfo* FindClassInfo(const String& typeName)
        {
            if (auto* info = Registry::Get().Find(typeName.c_str()))
                return info;

            const std::array<String, 7> namespaces = {
                "BixEngine::Game::", "BixEngine::Render::", "BixEngine::Physics::",
                "BixEngine::Audio::", "BixEngine::Core::", "BixEngine::Systems::", "BixEngine::Gui::"
            };

            for (const auto& ns : namespaces)
            {
                if (auto* info = Registry::Get().FindByQualifiedName((ns + typeName).c_str()))
                    return info;
            }
            
            LOG_WARNING("Actor::FindClassInfo: Failed to find class info for '" + typeName + "'");
            return nullptr;
        }

        Component* FindOrCreateComponentForLoad(Actor* actor, const String& typeName, size_t index)
        {
            auto& components = actor->GetComponents();

            if (index < components.size() && components[index]->GetTypeName() == typeName)
                return components[index].get();

            for (const auto& comp : components)
                if (comp->GetTypeName() == typeName) return comp.get();

            if (const auto* info = FindClassInfo(typeName))
            {
                if (info->CanConstruct())
                {
                    if (auto* newComp = info->ConstructTyped<Component>())
                    {
                        actor->AddComponent(std::unique_ptr<Component>(newComp));
                        return actor->GetComponents().back().get();
                    }
                }
            }
            
            return nullptr;
        }
    }

    
    
    
    

    Actor::Actor(String name, const Math::Transform& transform) : Object(std::move(name), transform)
    {
    }

    Actor::~Actor()
    {
        SetParent(nullptr);
        
        auto childrenCopy = children_;
        for (auto* child : childrenCopy)
        {
            if (child)
                child->SetParent(nullptr);
        }
    }

    void Actor::BeginPlay()
    {
        for (auto& c : components_)
            c->BeginPlay();
        
        hasBegunPlay_ = true;
    }

    void Actor::Update(float deltaTime)
    {
        if (!active_)
            return;

        if (!hasBegunPlay_)
            BeginPlay();

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

    
    
    
    

    void Actor::SetParent(Actor* parent)
    {
        if (parent_ == parent)
            return;
        
        if (parent == this)
            return;
        
        if (parent && parent->IsChildOf(this))
        {
            LOG_WARNING("Hierarchy cycle detected for actor: " + GetName());
            return;
        }

        if (parent_)
        {
            auto& siblings = parent_->children_;
            std::erase(siblings, this);
        }

        parent_ = parent;

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
        
        for (const Actor* curr = parent_; curr != nullptr; curr = curr->parent_)
        {
            if (curr == potentialParent)
                return true;
        }
        
        return false;
    }

    
    
    
    

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

        if (it != components_.end())
        {
            OnComponentRemoved(*(*it));
            components_.erase(it);
            
            return true;
        }
        
        return false;
    }

    
    
    
    

    std::unique_ptr<Actor> Actor::ClonePrototype() const
    {
        return std::make_unique<Actor>();
    }

    void Actor::SerializeBinary(std::ostream& stream) const
    {
        Object::SerializeBinary(stream);
        BinaryWriter writer(stream);

        writer.WriteString(parent_ ? parent_->GetUUID() : String());

        if (const auto* info = FindClassInfo(GetTypeName()))
        {
            Serialization::ReflectedSerializer::Serialize(this, info, writer);   
        }
        else
        {
            LOG_WARNING("Serialize: Missing ClassInfo for " + GetTypeName());   
        }

        SerializeComponents(stream);
    }

    void Actor::SerializeComponents(std::ostream& stream) const
    {
        BinaryWriter writer(stream);
        writer.WriteUint32(static_cast<uint32_t>(components_.size()));

        for (const auto& comp : components_)
        {
            String typeName = comp->GetTypeName();
            writer.WriteString(typeName);

            if (const auto* info = FindClassInfo(typeName))
            {
                
                Serialization::ReflectedSerializer::Serialize(comp.get(), info, writer);
            }
            else
            {
                LOG_ERROR("Actor::SerializeComponents: SKIPPING '" + typeName + "' - No ClassInfo found!");
            }
        }
    }

    void Actor::DeserializeBinary(std::istream& stream)
    {
        Object::DeserializeBinary(stream);
        BinaryReader reader(stream);

        reader.ReadString(parentUUID_);

        if (const auto* info = FindClassInfo(GetTypeName()))
            Serialization::ReflectedSerializer::Deserialize(this, info, reader);

        DeserializeComponents(stream);
    }

    void Actor::DeserializeComponents(std::istream& stream)
    {
        BinaryReader reader(stream);
        uint32_t count = 0;
        
        if (!reader.ReadUint32(count))
            return;

        for (uint32_t i = 0; i < count; ++i)
        {
            String typeName;
            if (!reader.ReadString(typeName))
                break;

            if (Component* comp = FindOrCreateComponentForLoad(this, typeName, i))
            {
                if (const auto* info = FindClassInfo(typeName))
                    Serialization::ReflectedSerializer::Deserialize(comp, info, reader);
            }
            else
            {
                LOG_ERROR("Deserialize: Failed to resolve component: " + typeName);
            }
        }
    }

    Math::Transform Actor::ComputeWorldTransform() const
    {
        Math::Transform t;
        const auto& selfT = GetTransformRef();
        t.SetPosition(selfT.GetWorldPosition());
        t.SetRotation(selfT.GetWorldRotation());
        t.SetScale(selfT.GetWorldScale());
        return t;
    }
}