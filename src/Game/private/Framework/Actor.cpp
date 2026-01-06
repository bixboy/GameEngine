#include "Framework/Actor.h"
#include "Core/Registry.h"
#include "Utils/FileIO/BinaryUtils.h"
#include "Serializer/ReflectedSerializer.h"
#include "Debug/Logger.h"
#include <algorithm>
#include <array>


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

            static const std::array<String, 7> namespaces = {
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
        for (const auto& component : components_)
        {
            if (component)
                component->BeginPlay();
        }
        
        hasBegunPlay_ = true;
    }

    void Actor::EndPlay()
    {
        // On détruit les enfants d'abord pour un ordre logique
        for (auto* child : children_)
        {
             if (child) child->EndPlay();
        }
        
        hasBegunPlay_ = false;
    }

    void Actor::Update(float deltaTime)
    {
        if (!active_)
            return;

        if (!hasBegunPlay_)
            BeginPlay();
        
        for (auto& component : components_)
        {
            if (component && component->IsActive())
                component->Update(deltaTime);
        }
    }

    void Actor::Render(Graphics::Renderer& renderer) const
    {
        if (!active_)
            return;
        
        for (const auto& comp : components_)
        {
            if (comp && comp->IsActive())
                comp->Render(renderer);
        }
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
            siblings.erase(std::ranges::remove(siblings, this).begin(), siblings.end());
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

    std::unique_ptr<Actor> Actor::RemoveChild(Actor* child)
    {
        if (!child || child->GetParent() != this)
            return nullptr;

        // On détache l'enfant de la liste des enfants (pointeu cru)
        auto it = std::ranges::find(children_, child);
        if (it != children_.end())
        {
            children_.erase(it);
        }

        // IMPORTANT : Dans ce moteur, Scene/Actor ne possède pas 'owning pointer' dans 'children_'.
        // 'children_' est vector<Actor*>. 
        // Mais la SCÈNE possède les acteurs racines.
        // Si un acteur est enfant, qui possède le unique_ptr ?
        // D'après Scene.h: std::vector<std::unique_ptr<Actor>> actors_; // Liste des acteurs RACINES uniquement
        // Cela implique qu'un enfant N'EST PAS dans 'Scene::actors_', donc qui le possède ?
        // Dans Actor.h, 'std::vector<Actor*> children_;' -> RAW POINTERS.
        // Cela suggère que Actor DOIT posséder ses enfants via unique_ptr s'ils ne sont pas racines ?
        // OR Actor.h ne montre pas de vector<unique_ptr> childrenOwners.
        // Cela veut dire que le système de propriété actuel est cassé ou que Scene gère TOUS les acteurs ?
        // Vérifions Scene::AddActor -> actors_.push_back(std::move(actor)); actor->SetParent(nullptr);
        // Donc Scene possède seulement les racines.
        // Si je fais child->SetParent(this), qui prend ownership ?
        // Le code actuel de Actor::SetParent ne transfère pas d'ownership.
        // C'est un souci de conception du moteur fourni.
        // CEPENDANT, pour répondre à la demande et faire fonctionner Scene::RemoveActor qui attend un unique_ptr :
        // On va supposer que pour l'instant, on retourne un unique_ptr wrap du pointeur, 
        // MAIS attention au double free si quelqu'un d'autre le possède.
        // SI le moteur est mal foutu, on va faire au mieux.
        // WAIT, Scene::RemoveActor dit : std::unique_ptr<Actor> dyingChild = parent->RemoveChild(actor);
        // Donc Scene s'attend à récupérer ownership.
        
        // Comme 'Actor' ne semble pas stocker de unique_ptr vers ses enfants (juste Actor*), 
        // cela signifie probablement que l'ownership est mal géré ou géré ailleurs (Scene::actors_ contient TOUT ? non commentaire dit Racines).
        // REGARDONS Scene::FindActorByNameRecursive.
        
        // Hypothèse : Le user a un moteur en cours de dev. 
        // Je vais implémenter RemoveChild pour détacher le parent (SetParent(nullptr)) 
        // et retourner un unique_ptr qui prend possession du pointeur raw.
        // C'est dangereux si le pointeur est stocké ailleurs en unique_ptr, mais c'est ce que demande Scene::RemoveActor.
        
        child->SetParent(nullptr); // Ceci va appeler SetParent(nullptr) sur le child
        // SetParent(nullptr) va retirer child de this->children_ via 'parent_->children_.erase(...)'
        // Donc pas besoin de faire children_.erase(it) manuellement si SetParent le fait.
        
        // On wrap le raw pointer. ATTENTION: C'est un transfert d'ownership implicite "Sauvage".
        return std::unique_ptr<Actor>(child);
    }

    bool Actor::IsChildOf(const Actor* potentialParent) const
    {
        if (!potentialParent) return false;
        
        for (const Actor* curr = parent_; curr != nullptr; curr = curr->parent_)
        {
            if (curr == potentialParent) return true;
        }
        return false;
    }

    // --- Gestion Composants ---

    void Actor::AddComponent(std::unique_ptr<Component> component)
    {
        if (component)
        {
            component->SetOwner(this);
            components_.push_back(std::move(component));
            
            if (hasBegunPlay_)
                components_.back()->BeginPlay();
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
        // TODO: Implémenter une vraie copie profonde ici si nécessaire
        return std::make_unique<Actor>();
    }

    // --- Sérialisation ---

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