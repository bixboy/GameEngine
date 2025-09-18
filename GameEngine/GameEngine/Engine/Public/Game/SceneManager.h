#pragma once

#include <memory>
#include <type_traits>
#include <utility>

#include "Game/Scene.h"

namespace Engine::Game {

class SceneManager {
public:
    void SetScene(std::unique_ptr<Scene> newScene) noexcept;
    void SetContext(SceneContext context) noexcept;

    [[nodiscard]] Scene* GetScene() noexcept { return scene_.get(); }
    [[nodiscard]] const Scene* GetScene() const noexcept { return scene_.get(); }

    template <typename TScene, typename... Args>
    TScene& EmplaceScene(Args&&... args) {
        static_assert(std::is_base_of_v<Scene, TScene>, "TScene must derive from Engine::Game::Scene");
        auto newScene = std::make_unique<TScene>(std::forward<Args>(args)...);
        TScene& sceneRef = *newScene;
        ActivateScene(std::move(newScene));
        return sceneRef;
    }

private:
    void ActivateScene(std::unique_ptr<Scene> newScene) noexcept;

    std::unique_ptr<Scene> scene_{};
    SceneContext context_{};
};

}  // namespace Engine::Game
