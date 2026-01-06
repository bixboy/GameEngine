#pragma once
#include <memory>
#include <vector>
#include <algorithm>
#include <SDL3/SDL_events.h>

#include "Actor.h"
#include "InputManager.h"
#include "Renderer.h"
#include "Containers/String.h"
#include "Components/Core/Component.h"
#include "Time/Timer.h"
#include "Systems/Core/Window.h"
#include <box2d/box2d.h>
#include "Scene.generated.h"


namespace BixEngine::Gui
{
    class GuiManager;
}


namespace BixEngine::Game
{
    class Actor;
    class CameraComponent;

    struct SceneContext
    {
        Graphics::Renderer* renderer{nullptr};
        Input::InputManager* inputManager{nullptr};
        Core::Window* window{nullptr};
        Core::Timer* timer{nullptr};
        Gui::GuiManager* guiManager{nullptr};
    };

    BCLASS()
    class Scene
    {
        GENERATED_BODY()

    public:
        explicit Scene(String name = "Unnamed Scene");
        virtual ~Scene();

        void SetName(String name);
        
        // --- Cycle de Vie ---
        virtual void OnInitialize() {}
        virtual void OnEnter() {}
        virtual void OnExit() {}
        virtual void OnPause() {}
        virtual void OnResume() {}

        void OnRuntimeStart();
        void OnRuntimeStop();
        
        virtual void OnEditorUpdate(float deltaTime);
        virtual void OnRuntimeUpdate(float deltaTime);

        // --- Événements ---
        virtual void OnWindowResize(int width, int height);
        virtual void OnFileDrop(const String& filePath);
        virtual void HandleEvent(const SDL_Event& event);
        
        // --- Rendering ---
        virtual void Render(Graphics::Renderer& renderer);
        virtual void PostRender(Graphics::Renderer& renderer);

        // --- Gestion des Acteurs ---
        void SetContext(SceneContext context) noexcept;
        void AddActor(std::unique_ptr<Actor> actor);
        void RemoveActor(Actor* actor);
        void ClearActors() noexcept;

        template<typename T, typename... Args>
        T& SpawnActor(Args&&... args)
        {
            static_assert(std::is_base_of_v<Actor, T>, "T must derive from Actor");
            auto actor = std::make_unique<T>(std::forward<Args>(args)...);
            T& ref = *actor;
            AddActor(std::move(actor));
            
            return ref;
        }
        
        [[nodiscard]] const std::vector<std::unique_ptr<Actor>>& GetActors() const noexcept { return actors_; }
        
        Actor* FindActorByName(const String& name) noexcept;
        
        Actor* FindActorByID(const String& uuid) noexcept;

        template<typename T>
        T* FindActorByType() noexcept
        {
            static_assert(std::is_base_of_v<Actor, T>, "T must derive from Actor");
            for (auto& a : actors_)
            {
                if (auto* found = FindActorByTypeRecursive<T>(a.get()))
                    return found;
            }
            
            return nullptr;
        }

        template<typename T>
        std::vector<T*> FindActorsByType() noexcept
        {
            static_assert(std::is_base_of_v<Actor, T>, "T must derive from Actor");
            std::vector<T*> result;
            for (auto& a : actors_)
            {
                CollectActorsByTypeRecursive<T>(a.get(), result);
            }
            
            return result;
        }

        // --- Getters / Setters ---
        void Rename(String name);
        [[nodiscard]] const String& GetName() const noexcept { return name_; }

        void SetSourcePath(const String& path);
        [[nodiscard]] const String& GetSourcePath() const noexcept { return sourcePath_; }

        [[nodiscard]] bool HasWindow() const { return context_.window != nullptr; }
        [[nodiscard]] bool HasRenderer() const { return context_.renderer != nullptr; }
        [[nodiscard]] bool HasInputManager() const { return context_.inputManager != nullptr; }
        [[nodiscard]] Core::Window& GetWindow() const;
        [[nodiscard]] Input::InputManager& GetInputManager() const;
        [[nodiscard]] Graphics::Renderer& GetRenderer() const;
        [[nodiscard]] Core::Timer& GetTimer() const;
        [[nodiscard]] Gui::GuiManager& GetGuiManager() const;

        [[nodiscard]] b2WorldId GetPhysicsWorld() const noexcept { return physicsWorldId_; }

        void SetActiveCamera(CameraComponent* camera) { activeCamera_ = camera; }
        [[nodiscard]] CameraComponent* GetActiveCamera() const { return activeCamera_; }

    private:
        template<typename T>
        T* FindActorByTypeRecursive(Actor* current)
        {
            if (!current)
                return nullptr;
            
            if (auto* ptr = dynamic_cast<T*>(current))
                return ptr;

            for (auto* child : current->GetChildren())
            {
                if (auto* found = FindActorByTypeRecursive<T>(child))
                    return found;
            }
            
            return nullptr;
        }

        template<typename T>
        void CollectActorsByTypeRecursive(Actor* current, std::vector<T*>& outResult)
        {
            if (!current)
                return;
            
            if (auto* ptr = dynamic_cast<T*>(current))
                outResult.push_back(ptr);

            for (auto* child : current->GetChildren())
            {
                CollectActorsByTypeRecursive<T>(child, outResult);
            }
        }

    private:
        String name_;
        String sourcePath_;
        SceneContext context_{};
        
        std::vector<std::unique_ptr<Actor>> actors_;
        std::vector<std::unique_ptr<Actor>> pendingDestruction_;
        
        b2WorldId physicsWorldId_ = b2_nullWorldId;
        float physicsAccumulator_ = 0.0f;

        CameraComponent* activeCamera_{nullptr};
    };
}
