#include "Game/Scene.h"

#include <stdexcept>
#include <utility>

#include "Core/Timer.h"
#include "Core/Window.h"
#include "Graphics/Renderer.h"
#include "Input/Input.h"

namespace Engine::Game {

Scene::Scene(std::string name) : name_(std::move(name)) {}

void Scene::SetContext(SceneContext context) noexcept {
    context_ = context;
}

void Scene::SetName(std::string name) {
    name_ = std::move(name);
}

Input::Input& Scene::GetInput() const {
    if (!context_.input) {
        throw std::runtime_error("Scene context does not provide an input subsystem.");
    }

    return *context_.input;
}

Graphics::Renderer& Scene::GetRenderer() const {
    if (!context_.renderer) {
        throw std::runtime_error("Scene context does not provide a renderer.");
    }

    return *context_.renderer;
}

Core::Window& Scene::GetWindow() const {
    if (!context_.window) {
        throw std::runtime_error("Scene context does not provide a window.");
    }

    return *context_.window;
}

Core::Timer& Scene::GetTimer() const {
    if (!context_.timer) {
        throw std::runtime_error("Scene context does not provide a timer.");
    }

    return *context_.timer;
}

}  // namespace Engine::Game
