#pragma once

#include "Game/Scene.h"
#include "TestScene.generated.h"

namespace BixEngine
{
    namespace Graphics { class Renderer; }

    namespace Game
    {
        class Player;

        BCLASS()
        class TestScene : public Scene
        {
            GENERATED_BODY()
            
            public:
                TestScene();
                ~TestScene() override;

                void OnEnter() override;
                void OnExit() override;
                void HandleEvent(const SDL_Event& event) override;
                void Update(float deltaTime) override;
                void Render(Graphics::Renderer& renderer) override;

            private:
                Player* player_{nullptr};
        };
    }
}

