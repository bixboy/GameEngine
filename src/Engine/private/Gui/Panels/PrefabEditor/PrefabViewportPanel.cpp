#include "Gui/Panels/PrefabEditor/PrefabViewportPanel.h"

namespace BixEngine::Gui
{
    PrefabViewportPanel::PrefabViewportPanel(
        std::function<SDL_Texture*()> textureProvider,
        std::function<std::pair<int, int>()> sizeProvider,
        std::function<Game::Scene*()> sceneProvider,
        std::function<Game::Actor*()> selectionGetter,
        std::function<void(Game::Actor*)> selectionSetter
    )
    : SceneViewportPanel(DefaultEngineGuiContext{
        .sceneManagerProvider = nullptr,
        .sceneProvider = std::move(sceneProvider),
        .selectedActorGetter = std::move(selectionGetter),
        .selectedActorSetter = std::move(selectionSetter),
        .sceneRenderTextureProvider = std::move(textureProvider),
        .sceneRenderTextureSizeProvider = std::move(sizeProvider)
    })
    {
        // Custom initialization if needed
    }
}
