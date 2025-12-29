#include "Gui/Core/GuiContextFactory.h"
#include "Debug/Logger.h"
#include "Containers/String.h"
#include "Systems/Core/SubsystemManager.h"
#include "Gui/Core/DefaultEngineGui.h"
#include "Framework/SceneManager.h"
#include <filesystem>

#include "Input.h"


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

    std::function<void(const std::filesystem::path&)> DefaultEngineGuiContextFactory::DefaultAssetOpener()
    {
        return [](const std::filesystem::path& path)
        {
            if (path.empty())
                return;

            LOG_INFO("Requested to open asset in editor: " + path.generic_string());
        };
    }

    DefaultEngineGuiContextFactory::DefaultEngineGuiContextFactory(Core::SubsystemManager& subsystems) : subsystems_(
        subsystems)
    {
    }

    
    
    
    DefaultEngineGuiContext DefaultEngineGuiContextFactory::CreateContext(const DefaultEngineGuiContextArgs& args) const
    {
        DefaultEngineGuiContext context{};

        
        
        
        if (!args.bEnableGui)
        {
            LOG_INFO("[GuiContextFactory] Mode headless activé : aucun contexte GUI créé.");
            return context;
        }

        
        
        
        context.timer = subsystems_.GetTimer();

        
        
        
        context.sceneManagerProvider = [manager = &subsystems_]() -> Game::SceneManager*
        {
            return manager ? manager->GetSceneManager() : nullptr;
        };

        
        
        
        if (!args.lastDeltaTime)
            LOG_WARNING("[GuiContextFactory] Aucun delta time fourni au contexte GUI.");

        context.lastDeltaTime = args.lastDeltaTime;

        
        
        
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

        
        
        
        context.sceneRenderTextureSizeProvider = args.sceneViewportSizeProvider ? args.sceneViewportSizeProvider : DefaultSizeProvider();

        
        
        
        context.openScriptFilesInEditor = args.openScriptFilesInEditor ? args.openScriptFilesInEditor : DefaultScriptOpener();
        context.openAssetInEditor = args.openAssetInEditor ? args.openAssetInEditor : DefaultAssetOpener();

        context.mouseStatsProvider = [manager = &subsystems_]() -> const Input::MouseStatistics*
        {
            if (!manager)
                return nullptr;

            if (Input::Input* input = manager->GetInputDevice())
                return &input->GetMouseStatistics();

            return nullptr;
        };

        LOG_INFO("[GuiContextFactory] ✅ Contexte GUI créé avec succès.");
        return context;
    }
}
