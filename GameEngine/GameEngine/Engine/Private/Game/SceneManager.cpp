#include "Game/SceneManager.h"

#include <utility>

namespace Engine::Game {

void SceneManager::SetScene(std::unique_ptr<Scene> newScene) noexcept {
    ActivateScene(std::move(newScene));
}

void SceneManager::SetContext(SceneContext context) noexcept {
    context_ = context;

    if (scene_) {
        scene_->SetContext(context_);
    }
}

void SceneManager::ActivateScene(std::unique_ptr<Scene> newScene) noexcept {
    if (scene_) {
        scene_->OnExit();
    }

    scene_ = std::move(newScene);

    if (scene_) {
        scene_->SetContext(context_);
        scene_->OnEnter();
    }
}

}  // namespace Engine::Game
