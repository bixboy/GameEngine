#include "../../Public/Core/SceneManager.h"

void SceneManager::SetScene(std::unique_ptr<Scene> newScene)
{
    scene = std::move(newScene);
}

Scene* SceneManager::GetScene()
{
    return scene.get();
}
