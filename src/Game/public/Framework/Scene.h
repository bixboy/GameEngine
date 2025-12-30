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
#include <box2d/box2d.h>


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
        
        void OnEditorUpdate(float deltaTime);
        void OnRuntimeUpdate(float deltaTime);

        // --- Événements Système ---
        
        // Appelé quand la fenêtre change de taille
        virtual void OnWindowResize(int width, int height);

        // Appelé quand un fichier est déposé
        virtual void OnFileDrop(const String& filePath);

        virtual void HandleEvent(const SDL_Event& event);
        
        // Rendering
        virtual void Render(Graphics::Renderer& renderer);
        virtual void PostRender(Graphics::Renderer& renderer);

        // --- Gestion des Acteurs ---

        void SetContext(SceneContext context) noexcept;

        // Ajoute un acteur racine
        void AddActor(std::unique_ptr<Actor> actor);
        
        // Supprime un acteur 
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

        // --- Requêtes ---

        [[nodiscard]] const std::vector<std::unique_ptr<Actor>>& GetActors() const noexcept { return actors_; }
        
        Actor* FindActorByName(const String& name) noexcept;
        Actor* FindActorByPath(const String& path) noexcept;

        template<typename T>
        T* FindActorByType() noexcept
        {
            static_assert(std::is_base_of_v<Actor, T>, "T must derive from Actor");
            for (auto& a : actors_)
            {
                // TODO: Recherche récursive dans les enfants si nécessaire
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
                // TODO: Ajouter recherche récursive
            }
            return result;
        }

        // --- Getters / Setters ---

        void Rename(String name);
        [[nodiscard]] const String& GetName() const noexcept { return name_; }

        void SetSourcePath(const String& path);
        [[nodiscard]] const String& GetSourcePath() const noexcept { return sourcePath_; }

        [[nodiscard]] Core::Window& GetWindow() const;
        [[nodiscard]] Input::InputManager& GetInputManager() const;
        [[nodiscard]] Graphics::Renderer& GetRenderer() const;
        [[nodiscard]] Core::Timer& GetTimer() const;
        [[nodiscard]] Gui::GuiManager& GetGuiManager() const;

        [[nodiscard]] b2WorldId GetPhysicsWorld() const noexcept { return physicsWorldId_; }

        // Gestion Caméra Active
        void SetActiveCamera(CameraComponent* camera) { activeCamera_ = camera; }
        [[nodiscard]] CameraComponent* GetActiveCamera() const { return activeCamera_; }

    private:
        String name_;
        String sourcePath_;
        SceneContext context_{};
        
        // Liste des acteurs RACINES uniquement
        std::vector<std::unique_ptr<Actor>> actors_;
        
        // File d'attente pour suppression sécurisée
        std::vector<std::unique_ptr<Actor>> pendingDestruction_;
        
        b2WorldId physicsWorldId_ = b2_nullWorldId;
        float physicsAccumulator_ = 0.0f;

        CameraComponent* activeCamera_{nullptr};
    };
}