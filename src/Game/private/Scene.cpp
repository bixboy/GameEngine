#include "Scene.h"
#include <stdexcept>
#include <string_view>
#include <utility>
#include "Actor.h"
#include "Timer.h"
#include "Systems/Window.h"
#include "Renderer.h"
#include "Gui/GuiManager.h"
#include "Logger.h"
#include "Serializer/SceneSerializer.h"


namespace BixEngine::Game
{
    Scene::Scene(String name) : name_(std::move(name))
    {
        LOG_INFO("Scene created: " + name_);
    }

    void Scene::SetContext(SceneContext context) noexcept
    {
        context_ = context;
    }

    void Scene::AddActor(std::unique_ptr<Actor> actor)
    {
        if (!actor)
            return;

        Serialization::SceneSerializer::EnsureActorFactory(*actor);
        actor->SetOwningScene(this);

        actors_.push_back(std::move(actor));
    }

    void Scene::RemoveActor(Actor* actor)
    {
        if (!actor) return;

        std::erase_if(actors_, [&](const std::unique_ptr<Actor>& ptr)
        {
            if (ptr.get() == actor)
            {
                actor->SetOwningScene(nullptr);
                return true;
            }
            return false;
        });
    }

    void Scene::ClearActors() noexcept
    {
        for (auto& actor : actors_)
        {
            if (actor)
                actor->SetOwningScene(nullptr);
        }

        actors_.clear();
    }

    Actor* Scene::FindActorByName(const String& name) noexcept
    {
        for (auto& a : actors_)
        {
            if (a && a->GetName() == name)
                return a.get();
        }
        return nullptr;
    }

    Actor* Scene::FindActorByPath(const String& path) noexcept
    {
        if (path.IsEmpty())
            return nullptr;

        std::string_view view = path.View();
        const std::size_t separator = view.find_last_of("/\\");
        if (separator != std::string_view::npos)
            view = view.substr(separator + 1);

        if (view.empty())
            return nullptr;

        const String actorName(view);
        return FindActorByName(actorName);
    }

    void Scene::Rename(String name)
    {
        name_ = std::move(name);
    }

    void Scene::SetName(String name)
    {
        name_ = std::move(name);
    }

    // ────────────────────────────────────────────────
    // 🧠 Accès aux sous-systèmes
    // ────────────────────────────────────────────────

    Input::InputManager& Scene::GetInputManager() const
    {
        if (!context_.inputManager)
            throw std::runtime_error("Scene context does not provide an input subsystem.");

        return *context_.inputManager;
    }

    Graphics::Renderer& Scene::GetRenderer() const
    {
        if (!context_.renderer)
            throw std::runtime_error("Scene context does not provide a renderer.");

        return *context_.renderer;
    }

    Core::Window& Scene::GetWindow() const
    {
        if (!context_.window)
            throw std::runtime_error("Scene context does not provide a window.");

        return *context_.window;
    }

    Core::Timer& Scene::GetTimer() const
    {
        if (!context_.timer)
            throw std::runtime_error("Scene context does not provide a timer.");

        return *context_.timer;
    }

    Gui::GuiManager& Scene::GetGuiManager() const
    {
        if (!context_.guiManager)
            throw std::runtime_error("Scene context does not provide a GUI manager.");

        return *context_.guiManager;
    }
    void Scene::OnRuntimeStart()
    {
        bool hasInput = HasInputManager();
        Input::InputManager* inputManager = hasInput ? &GetInputManager() : nullptr;

        for (auto& actor : actors_)
        {
            if (actor && actor->IsActive())
            {
                actor->BeginPlay();
                if (inputManager)
                    actor->SetupInput(*inputManager);
            }
        }
    }

    void Scene::OnRuntimeStop()
    {
        // Reset logic if needed
    }

    void Scene::OnEditorUpdate(float deltaTime)
    {
        (void)deltaTime;
        // Editor specific updates (e.g. editor camera if managed here)
    }

    void Scene::OnRuntimeUpdate(float deltaTime)
    {
        for (auto& actor : actors_)
        {
            if (actor && actor->IsActive())
                actor->Update(deltaTime);
        }
    }
}
