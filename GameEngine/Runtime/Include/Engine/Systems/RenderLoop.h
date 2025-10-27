#pragma once

#include "Core/Math/Color.h"

namespace BixEngine
{
    namespace Graphics { class Renderer; }
    namespace Core { class SubsystemManager; class GuiModule; }
}

namespace BixEngine::Core
{
    class RenderLoop
    {
        
    public:
        
        /**
        * @brief Configure la boucle de rendu.
        * 
        * Doit être appelée avant le premier frame pour lier les modules essentiels.
        * 
        * @param subsystems    Pointeur vers le gestionnaire des sous-systèmes.
        * @param guiModule     Pointeur vers le module d'interface utilisateur.
        * @param renderer      Pointeur vers le moteur de rendu.
        * @param clearColor    Couleur utilisée pour effacer l'écran chaque frame.
        */
        void Configure(SubsystemManager* subsystems, GuiModule* guiModule, Graphics::Renderer* renderer, Math::Color clearColor) noexcept;


        /**
        * @brief Réinitialise la boucle à son état par défaut.
        * 
        * Tous les pointeurs sont remis à nullptr, le delta time est réinitialisé.
        * Redémarrage ou de déchargement du moteur.
        */
        void Reset() noexcept;

        
        /**
        * @brief Calcule le delta time de la frame en cours.
        * 
        * Met à jour le Timer via SubsystemManager, et stocke le temps écoulé
        * depuis la dernière frame dans lastDeltaTime_.
        * 
        * @return Temps écoulé en secondes depuis la dernière frame.
        */
        float CalculateDeltaTime();


        /**
        * @brief Démarre une nouvelle frame graphique.
        * 
        * Notifie le module GUI qu'une nouvelle frame commence (ImGui::NewFrame, etc.).
        */
        void BeginFrame();

        
        /**
        * @brief Met à jour tous les sous-systèmes pour cette frame.
        * 
        * Appelle SubsystemManager::UpdateAll(deltaTime) pour propager la mise à jour
        * à la logique de jeu, la physique, les entrées, etc.
        * 
        * @param deltaTime Temps écoulé depuis la dernière frame.
        */
        void Update(float deltaTime);


        /**
        * @brief Effectue le rendu complet de la frame.
        * 
        * Séquence typique :
        *  - Rendu de la scène dans une texture (si GUI actif) ou directement à l'écran.
        *  - Rendu du GUI par-dessus la scène.
        *  - Présentation finale (swap buffer).
        */
        void Render();


        /**
        * @brief Retourne un pointeur constant vers le dernier delta time calculé.
        * 
        * Permet de référencer la valeur sans la copier.
        * 
        * @return Pointeur vers lastDeltaTime_.
        */
        const float* GetLastDeltaTimePointer() const noexcept { return &lastDeltaTime_; }

    private:
        SubsystemManager* subsystems_{nullptr};
        
        GuiModule* guiModule_{nullptr};
        
        Graphics::Renderer* renderer_{nullptr};

        /** Couleur utilisée pour nettoyer le framebuffer à chaque frame. */
        Math::Color clearColor_{0, 0, 0, 255};

        /** Temps écoulé entre deux frames consécutives. */
        float lastDeltaTime_{0.0f};
    };
}
