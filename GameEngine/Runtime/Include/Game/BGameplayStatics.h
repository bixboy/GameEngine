#pragma once

#include "Core/Containers/String.h"

namespace BixEngine::Game
{
    class Scene;
    class SceneManager;

    class BGameplayStatics
    {
    public:
        static void OpenScene(void* worldContext, const String& sceneName);
        static void LoadScene(void* worldContext, const String& sceneName);
        static void UnloadScene(void* worldContext, const String& sceneName);
        static void PreloadScene(void* worldContext, const String& sceneName);

        [[nodiscard]] static Scene* GetActiveScene() noexcept;
        [[nodiscard]] static SceneManager* ResolveSceneManager(void* worldContext);
    };
}

