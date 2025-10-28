#include "Engine/Gui/GuiContextFactory.h"

#include "Core/Logger.h"
#include "Core/Containers/String.h"
#include "Engine/Systems/SubsystemManager.h"
#include "Engine/Gui/DefaultEngineGui.h"
#include "Game/SceneManager.h"

#include <filesystem>

namespace BixEngine::Gui
{

    std::function<std::pair<int, int>()> DefaultEngineGuiContextFactory::DefaultSizeProvider()
    {
        return []() -> std::pair<int, int> { return {0, 0}; };
    }

    std::function<void(const std::vector<std::filesystem::path>&)> DefaultEngineGuiContextFactory::DefaultScriptOpener()
    {
        return [](const std::vector<std::filesystem::path>& paths)
        {
            if (paths.empty())
                return;

            String message = "Requested to open script files in code editor:";
            for (const auto& path : paths)
            {
                message += "\n - ";
                message += path.generic_string();
            }

            LOG_INFO(message);
        };
    }
    
    DefaultEngineGuiContextFactory::DefaultEngineGuiContextFactory(Core::SubsystemManager& subsystems): subsystems_(subsystems)
    {
    }

    // ────────────────────────────────────────────────
    // 🧠 Création du contexte GUI principal
    // ────────────────────────────────────────────────
    DefaultEngineGuiContext DefaultEngineGuiContextFactory::CreateContext(const DefaultEngineGuiContextArgs& args) const
    {
        DefaultEngineGuiContext context{};

        // ──────────────────────────────
        // Mode headless
        // ──────────────────────────────
        if (!args.bEnableGui)
        {
            LOG_INFO("[GuiContextFactory] Mode headless activé : aucun contexte GUI créé.");
            return context;
        }

        // ──────────────────────────────
        // Timer du moteur
        // ──────────────────────────────
        context.timer = subsystems_.GetTimer();

        // ──────────────────────────────
        // Fournisseur SceneManager
        // ──────────────────────────────
        context.sceneManagerProvider = [manager = &subsystems_]() -> Game::SceneManager*
        {
            return manager ? manager->GetSceneManager() : nullptr;
        };

        // ──────────────────────────────
        // Référence au deltaTime
        // ──────────────────────────────
        if (!args.lastDeltaTime)
            LOG_WARNING("[GuiContextFactory] Aucun delta time fourni au contexte GUI.");
        
        context.lastDeltaTime = args.lastDeltaTime;

        // ──────────────────────────────
        // Acteur sélectionné
        // ──────────────────────────────
        if (args.selectedActorSlot)
        {
            context.selectedActorGetter = [slot = args.selectedActorSlot]() -> Game::Actor*
            {
                return (slot && *slot) ? *slot : nullptr;
            };

            context.selectedActorSetter = [slot = args.selectedActorSlot](Game::Actor* actor)
            {
                if (slot)
                    *slot = actor;
            };
        }
        else
        {
            LOG_WARNING("[GuiContextFactory] Aucun pointeur d’acteur sélectionné fourni (selectedActorSlot=nullptr).");
        }

        // ──────────────────────────────
        // Texture du viewport
        // ──────────────────────────────
        if (args.sceneViewportTexture)
        {
            context.sceneRenderTextureProvider = [texture = args.sceneViewportTexture]() -> SDL_Texture*
            {
                return texture ? *texture : nullptr;
            };
        }
        else
        {
            LOG_WARNING("[GuiContextFactory] Aucun viewport texture fourni.");
            context.sceneRenderTextureProvider = []() -> SDL_Texture* { return nullptr; };
        }

        // ──────────────────────────────
        // Taille viewport
        // ──────────────────────────────
        context.sceneRenderTextureSizeProvider = args.sceneViewportSizeProvider ? args.sceneViewportSizeProvider : DefaultSizeProvider();

        // ──────────────────────────────
        // Ouvrerture scripts
        // ──────────────────────────────
        context.openScriptFilesInEditor = args.openScriptFilesInEditor ? args.openScriptFilesInEditor : DefaultScriptOpener();

        LOG_INFO("[GuiContextFactory] ✅ Contexte GUI créé avec succès.");
        return context;
    }
}
