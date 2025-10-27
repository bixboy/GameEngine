#pragma once

#include <memory>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "Core/Containers/String.h"
#include "Game/Scene.h"

namespace BixEngine::Game
{
    class SceneManager
    {
    public:
        SceneManager();
        ~SceneManager();

        SceneManager(const SceneManager&) = delete;
        SceneManager& operator=(const SceneManager&) = delete;
        SceneManager(SceneManager&&) noexcept = delete;
        SceneManager& operator=(SceneManager&&) noexcept = delete;

        // ────────────────────────────────────────────────
        // 🧱  Gestion du contexte et des scènes
        // ────────────────────────────────────────────────

        void SetScene(std::unique_ptr<Scene> newScene) noexcept;
        void SetContext(SceneContext context) noexcept;

        [[nodiscard]] Scene* GetScene() noexcept { return activeScene_.get(); }
        [[nodiscard]] const Scene* GetScene() const noexcept { return activeScene_.get(); }

        [[nodiscard]] bool HasScene() const noexcept { return activeScene_ != nullptr; }

        [[nodiscard]] static Scene* GetActiveScene() noexcept
        {
            return activeManager_ ? activeManager_->GetScene() : nullptr;
        }

        [[nodiscard]] static SceneManager* GetActiveSceneManager() noexcept { return activeManager_; }

        // ────────────────────────────────────────────────
        // 🧩  Création et activation directe
        // ────────────────────────────────────────────────

        template <typename TScene, typename... Args>
        TScene& EmplaceScene(Args&&... args)
        {
            static_assert(std::is_base_of_v<Scene, TScene>, "TScene must derive from BixEngine::Game::Scene");

            auto newScene = std::make_unique<TScene>(std::forward<Args>(args)...);
            TScene& sceneRef = *newScene;

            ActivateScene(std::move(newScene), sceneRef.GetName(), false);
            return sceneRef;
        }

        // ────────────────────────────────────────────────
        // 🔁  Contrôle du cycle de vie
        // ────────────────────────────────────────────────

        void ChangeSceneByName(const String& name);
        void LoadScene(const String& name);
        void UnloadScene(const String& name);
        void ReloadScene();
        void PreloadScene(const String& name) { LoadScene(name); }

        [[nodiscard]] const String& GetActiveSceneName() const noexcept { return activeSceneName_; }

    private:
        struct LoadedScene
        {
            std::unique_ptr<Scene> instance{};
            bool initialized{false};
        };

        void ActivateScene(std::unique_ptr<Scene> newScene, const String& name, bool alreadyInitialized);
        void DestroyActiveScene() noexcept;
        [[nodiscard]] LoadedScene AcquireScene(const String& name);
        void ApplyContext(Scene& scene) const noexcept;

        inline static SceneManager* activeManager_{nullptr};

        std::unique_ptr<Scene> activeScene_{};
        bool activeSceneInitialized_{false};
        String activeSceneName_{};
        std::unordered_map<String, LoadedScene> loadedScenes_{};
        SceneContext context_{};
    };
}
