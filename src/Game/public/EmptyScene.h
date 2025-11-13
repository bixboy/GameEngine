#pragma once
#include "Scene.h"
#include "EmptyScene.generated.h"

namespace BixEngine::Game
{
    class Player;

    BCLASS()

    class EmptyScene : public Scene
    {
        GENERATED_BODY()

    public:
        EmptyScene();

        void HandleEvent(const SDL_Event& event) override;
        void Update(float deltaTime) override;
        void Render(Graphics::Renderer& renderer) override;

        void OnEnter() override;
        void OnExit() override;

    private:
        Player* player_{nullptr};
    };
}
