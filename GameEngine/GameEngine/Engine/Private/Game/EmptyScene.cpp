#include "../../Public/Game/EmptyScene.h"


EmptyScene::EmptyScene()
    : player(300.f, 220.f, 50.f, 50.f, SDL_Color{255,255,255,255}) {}

void EmptyScene::HandleEvent(const SDL_Event& e)
{
    input.ProcessEvent(e);
}

void EmptyScene::Update(float dt)
{
    Vector3 pos = player.GetPosition();

    if (input.IsKeyDown(SDLK_Z)) pos.y -= speed * dt;
    if (input.IsKeyDown(SDLK_S)) pos.y += speed * dt;
    if (input.IsKeyDown(SDLK_Q)) pos.x -= speed * dt;
    if (input.IsKeyDown(SDLK_D)) pos.x += speed * dt;

    player.SetPosition(pos);
}

void EmptyScene::Render(Renderer& renderer)
{
    player.Render(renderer);
}
