#include "Game/Scene.h"

#include <stdexcept>
#include <utility>

#include "Core/Timer.h"
#include "Core/Window.h"
#include "Graphics/Renderer.h"
#include "Input/Input.h"

namespace Engine::Game
{
    Scene::Scene(std::string name) : name_(std::move(name)) {}

    void Scene::SetContext(SceneContext context) noexcept
    {
        context_ = context;
    }

    void Scene::AddActor(std::unique_ptr<Engine::Actor> actor)
    {
        actors_.push_back(std::move(actor));
    }

    void Scene::SetName(std::string name)
    {
        name_ = std::move(name);
    }

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
}
