#pragma once

#include <memory>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "Core/Containers/String.h"
#include "Game/Scene.h"

namespace BixEngine::Game
{
    /**
     * @class SceneManager
     * @brief Gère le cycle de vie complet des scènes.
     */
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

        /**
         * @brief Remplace la scène active actuelle par une nouvelle.
         * Appelle OnExit() sur l’ancienne, puis OnEnter() sur la nouvelle.
         */
        void SetScene(std::unique_ptr<Scene> newScene) noexcept;

        /**
         * @brief Définit le contexte moteur (renderer, input, timer...).
         * Ce contexte sera propagé à la scène active et à toutes les scènes préchargées.
         */
        void SetContext(SceneContext context) noexcept;

        /// Retourne la scène actuellement active.
        [[nodiscard]] Scene* GetScene() noexcept { return activeScene_.get(); }

        /// Version const de GetScene().
        [[nodiscard]] const Scene* GetScene() const noexcept { return activeScene_.get(); }

        /// Indique si une scène active est actuellement chargée.
        [[nodiscard]] bool HasScene() const noexcept { return activeScene_ != nullptr; }

        /// Retourne la scène active globale (shortcut statique).
        [[nodiscard]] static Scene* GetActiveScene() noexcept
        {
            return activeManager_ ? activeManager_->GetScene() : nullptr;
        }

        /// Retourne le gestionnaire global actuellement actif.
        [[nodiscard]] static SceneManager* GetActiveSceneManager() noexcept { return activeManager_; }

        // ────────────────────────────────────────────────
        // 🧩  Création et activation directe
        // ────────────────────────────────────────────────

        /**
         * @brief Crée et active une nouvelle scène directement depuis son type.
         *
         * @tparam TScene Type de la scène à instancier (doit dériver de Scene)
         * @param args Arguments transmis au constructeur de la scène
         */
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

        /**
         * @brief Change la scène active à partir de son nom (via le SceneRegistry).
         * Détruit proprement l’ancienne scène avant d’activer la nouvelle.
         */
        void ChangeSceneByName(const String& name);

        /**
         * @brief Précharge une scène en mémoire sans la rendre active.
         * Appelle OnInitialize() mais pas OnEnter().
         */
        void LoadScene(const String& name);

        /**
         * @brief Supprime une scène préchargée de la mémoire.
         * Ne peut pas décharger la scène active.
         */
        void UnloadScene(const String& name);

        /**
         * @brief Recharge entièrement la scène active actuelle.
         * Détruit puis recrée la scène par son nom.
         */
        void ReloadScene();

        /**
         * @brief Alias de LoadScene(), maintenu pour compatibilité conceptuelle.
         */
        void PreloadScene(const String& name) { LoadScene(name); }

        /// Retourne le nom de la scène actuellement active.
        [[nodiscard]] const String& GetActiveSceneName() const noexcept { return activeSceneName_; }

    private:
        // ────────────────────────────────────────────────
        // ⚙️  Structures internes
        // ────────────────────────────────────────────────

        /**
         * @brief Représente une scène préchargée dans le cache.
         */
        struct LoadedScene
        {
            // Instance de la scène en mémoire
            std::unique_ptr<Scene> instance{};

            // True si OnInitialize() a déjà été appelé
            bool initialized{false};
        };

        // ────────────────────────────────────────────────
        // ⚙️  Méthodes internes
        // ────────────────────────────────────────────────

        /**
         * @brief Active une scène donnée et met à jour le cycle de vie.
         * Appelle OnInitialize() si nécessaire, puis OnEnter().
         */
        void ActivateScene(std::unique_ptr<Scene> newScene, const String& name, bool alreadyInitialized);

        /**
         * @brief Détruit proprement la scène active (OnExit + reset).
         */
        void DestroyActiveScene() noexcept;

        /**
         * @brief Récupère une scène depuis le cache ou la crée via SceneRegistry.
         */
        [[nodiscard]] LoadedScene AcquireScene(const String& name);

        /**
         * @brief Applique le contexte moteur (renderer, input...) à une scène donnée.
         */
        void ApplyContext(Scene& scene) const noexcept;

        // ────────────────────────────────────────────────
        // 🗃️  Données membres
        // ────────────────────────────────────────────────

        // Instance globale du SceneManager actif
        inline static SceneManager* activeManager_{nullptr};

        // Scène actuellement active
        std::unique_ptr<Scene> activeScene_{};

        // Indique si la scène active a été initialisée
        bool activeSceneInitialized_{false};

        // Nom de la scène active
        String activeSceneName_{};

        // Scènes préchargées
        std::unordered_map<String, LoadedScene> loadedScenes_{};

        // Contexte moteur partagé
        SceneContext context_{};
    };
}
