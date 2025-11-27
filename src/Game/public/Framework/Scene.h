#pragma once
#include <memory>
#include <vector>
#include <algorithm>
#include <SDL3/SDL_events.h>

#include "InputManager.h"
#include "Renderer.h"
#include "Containers/String.h"
#include "Components/Core/Component.h"
#include "Time/Timer.h"
#include "Gui/Core/GuiManager.h"
#include "Systems/Core/Window.h"

#include "Scene.generated.h"


namespace BixEngine::Game
{
    class Actor;

    // Contexte passé à chaque scène par le moteur
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
        virtual ~Scene() = default;

        void SetName(String name);
        
        // Lifecycle
        virtual void OnInitialize() {}
        virtual void OnEnter() {}
        virtual void OnExit() {}
        virtual void OnPause() {}
        virtual void OnResume() {}

        void OnRuntimeStart();
        void OnRuntimeStop();
        void OnEditorUpdate(float deltaTime);
        void OnRuntimeUpdate(float deltaTime);

        // Loop
        virtual void HandleEvent(const SDL_Event& event) { (void)event; }
        virtual void Update(float deltaTime) { (void)deltaTime; }
        virtual void LateUpdate(float deltaTime) { (void)deltaTime; }
        virtual void Render(Graphics::Renderer& renderer) { (void)renderer; }
        virtual void PostRender(Graphics::Renderer& renderer) { (void)renderer; }

        // Context
        void SetContext(SceneContext context) noexcept;

        // Actors
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
        [[nodiscard]] std::vector<std::unique_ptr<Actor>>& GetActors() noexcept { return actors_; }

        Actor* FindActorByName(const String& name) noexcept;
        Actor* FindActorByPath(const String& path) noexcept;

        template<typename T>
        T* FindActorByType() noexcept
        {
            static_assert(std::is_base_of_v<Actor, T>, "T must derive from Actor");
            for (auto& a : actors_)
            {
                if (auto* ptr = dynamic_cast<T*>(a.get()))
                    return ptr;   
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
                if (auto* ptr = dynamic_cast<T*>(a.get()))
                    result.push_back(ptr);
            }
            
            return result;
        }

        // Name
        void Rename(String name);
        [[nodiscard]] const String& GetName() const noexcept
        {
            return name_;
        }

    protected:

        // Safe subsystem access
        [[nodiscard]] Input::InputManager& GetInputManager() const;
        [[nodiscard]] bool HasInputManager() const noexcept { return context_.inputManager != nullptr; }

        [[nodiscard]] Graphics::Renderer& GetRenderer() const;
        [[nodiscard]] bool HasRenderer() const noexcept { return context_.renderer != nullptr; }

        [[nodiscard]] Core::Window& GetWindow() const;
        [[nodiscard]] bool HasWindow() const noexcept { return context_.window != nullptr; }

        [[nodiscard]] Core::Timer& GetTimer() const;
        [[nodiscard]] bool HasTimer() const noexcept { return context_.timer != nullptr; }

        [[nodiscard]] Gui::GuiManager& GetGuiManager() const;
        [[nodiscard]] bool HasGuiManager() const noexcept { return context_.guiManager != nullptr; }

    private:
        String name_;
        SceneContext context_{};
        std::vector<std::unique_ptr<Actor>> actors_;
    };
}
