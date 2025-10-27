#pragma once

#include <memory>
#include <type_traits>
#include <utility>
#include <stack>

#include "Game/Scene.h"

namespace BixEngine::Game
{
    class SceneManager
    {
    public:
        SceneManager() = default;
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

        [[nodiscard]] Scene* GetScene() noexcept { return scene_.get(); }
        [[nodiscard]] const Scene* GetScene() const noexcept { return scene_.get(); }

        [[nodiscard]] static Scene* GetActiveScene() noexcept { return activeScene_; }
        [[nodiscard]] bool HasScene() const noexcept { return scene_ != nullptr; }

        // ────────────────────────────────────────────────
        // 🧩  Création et activation directe
        // ────────────────────────────────────────────────

        template <typename TScene, typename... Args>
        TScene& EmplaceScene(Args&&... args)
        {
            static_assert(std::is_base_of_v<Scene, TScene>, "TScene must derive from BixEngine::Game::Scene");

            auto newScene = std::make_unique<TScene>(std::forward<Args>(args)...);
            TScene& sceneRef = *newScene;

            ActivateScene(std::move(newScene));
            return sceneRef;
        }

        // ────────────────────────────────────────────────
        // 🔁  Contrôle du cycle de vie
        // ────────────────────────────────────────────────

        // Recrée la scène active
        void ReloadScene();

        // Empile une scène
        void PushScene(std::unique_ptr<Scene> newScene);

        // Retour à la scène précédente
        void PopScene();

        // Vide tout l’historique
        void ClearHistory();

    private:
        void ActivateScene(std::unique_ptr<Scene> newScene) noexcept;

        inline static Scene* activeScene_ = nullptr;

        std::unique_ptr<Scene> scene_{};
        
        SceneContext context_{};
        
        std::stack<std::unique_ptr<Scene>> history_{};
    };
}
