#pragma once
#include "Framework/Object.h"
#include "Math/Transform.h"
#include "Math/Vector2.h"
#include "Components/Core/Component.h"
#include <vector>
#include <memory>
#include <type_traits>
#include "Actor.generated.h"

namespace BixEngine::Input { class InputManager; }

namespace BixEngine::Game
{
    class Scene;

    BCLASS()
    class Actor : public Object
    {
        GENERATED_BODY()

    public:
        explicit Actor(String name = "Actor", const Math::Transform& transform = Math::Transform());
        virtual ~Actor();

        virtual void BeginPlay();
        virtual void EndPlay();
        virtual void Update(float deltaTime);
        virtual void Render(Graphics::Renderer& renderer) const;
        virtual void SetupInput(BixEngine::Input::InputManager& inputManager) { (void)inputManager; }

        // --- Hiérarchie ---
        void SetParent(Actor* parent);
        [[nodiscard]] std::unique_ptr<Actor> RemoveChild(Actor* child);
        [[nodiscard]] Actor* GetParent() const noexcept { return parent_; }
        [[nodiscard]] const std::vector<Actor*>& GetChildren() const noexcept { return children_; }
        
        [[nodiscard]] bool IsChildOf(const Actor* potentialParent) const;

        Math::Transform ComputeWorldTransform() const;

        // --- Gestion des Composants ---
        void AddComponent(std::unique_ptr<Component> component);
        bool RemoveComponent(const Component* component);

        [[nodiscard]] const std::vector<std::unique_ptr<Component>>& GetComponents() const noexcept { return components_; }
        
        template <typename T>
        T* GetComponent() const noexcept
        {
            static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
            
            for (const auto& comp : components_)
            {
                if (auto* casted = dynamic_cast<T*>(comp.get()))
                    return casted;
            }
            return nullptr;
        }

        template <typename T, typename... Args>
        T* AddComponent(Args&&... args)
        {
            static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
            
            auto comp = std::make_unique<T>(this, std::forward<Args>(args)...);
            T* ptr = comp.get();
            AddComponent(std::move(comp));
            
            return ptr;
        }

        // --- Sérialisation ---
        void SerializeBinary(std::ostream& stream) const override;
        void DeserializeBinary(std::istream& stream) override;

        [[nodiscard]] virtual std::unique_ptr<Actor> ClonePrototype() const;
        
        [[nodiscard]] String GetTypeName() const noexcept override { return GetClass().Name.c_str(); }

        // --- État ---
        [[nodiscard]] bool IsActive() const noexcept { return active_; }
        void SetActive(bool active) noexcept { active_ = active; }

        void MarkAsPendingKill() noexcept { pendingKill_ = true; }
        [[nodiscard]] bool IsPendingKill() const noexcept { return pendingKill_; }

        void SetOwningScene(Scene* scene) noexcept { owningScene_ = scene; }
        [[nodiscard]] Scene* GetOwningScene() const noexcept { return owningScene_; }

        [[nodiscard]] const String& GetLoadedParentUUID() const noexcept { return parentUUID_; }

    protected:
        virtual void OnComponentRemoved(const Component&) {}

    private:
        void SerializeComponents(std::ostream& stream) const;
        void DeserializeComponents(std::istream& stream);

        std::vector<std::unique_ptr<Component>> components_;
        
        Actor* parent_{nullptr};
        std::vector<Actor*> children_;
        
        String parentUUID_;
        
        Scene* owningScene_{nullptr};
        bool hasBegunPlay_{false};
        bool active_{true};
        bool pendingKill_{false};
    };
}