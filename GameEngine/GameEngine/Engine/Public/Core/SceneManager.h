#pragma once
#include "memory"
#include "Scene.h"

class SceneManager {
    
public:
    void SetScene(std::unique_ptr<Scene> newScene);

    Scene* GetScene();

private:
    std::unique_ptr<Scene> scene; 
};
