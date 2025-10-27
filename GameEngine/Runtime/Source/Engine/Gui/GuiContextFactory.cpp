#include "Engine/Gui/GuiContextFactory.h"

#include "Core/Containers/String.h"
#include "Core/Logger.h"
#include "Engine/Systems/SubsystemManager.h"
#include "Engine/Gui/DefaultEngineGui.h"
#include "Game/SceneManager.h"

#include <filesystem>

namespace BixEngine::Gui
{
    DefaultEngineGuiContextFactory::DefaultEngineGuiContextFactory(Core::SubsystemManager& subsystems)
        : subsystems_(subsystems)
    {
    }

    DefaultEngineGuiContext DefaultEngineGuiContextFactory::CreateContext(const DefaultEngineGuiContextArgs& args) const
    {
        DefaultEngineGuiContext context{};
        context.timer = subsystems_.GetTimer();
        context.sceneManagerProvider = [manager = &subsystems_]() -> Game::SceneManager*
        {
            return manager->GetSceneManager();
        };
        context.lastDeltaTime = args.lastDeltaTime;

        if (args.selectedActorSlot)
        {
            context.selectedActorGetter = [slot = args.selectedActorSlot]() -> Game::Actor*
            {
                return slot ? *slot : nullptr;
            };
            context.selectedActorSetter = [slot = args.selectedActorSlot](Game::Actor* actor)
            {
                if (slot)
                    *slot = actor;
            };
        }

        if (args.sceneViewportTexture)
        {
            context.sceneRenderTextureProvider = [texture = args.sceneViewportTexture]() -> SDL_Texture*
            {
                return texture ? *texture : nullptr;
            };
        }

        if (args.sceneViewportSizeProvider)
            context.sceneRenderTextureSizeProvider = args.sceneViewportSizeProvider;
        else
            context.sceneRenderTextureSizeProvider = []() -> std::pair<int, int> { return {0, 0}; };

        if (args.openScriptFilesInEditor)
        {
            context.openScriptFilesInEditor = args.openScriptFilesInEditor;
        }
        else
        {
            context.openScriptFilesInEditor = [](const std::vector<std::filesystem::path>& paths)
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

        return context;
    }
}

