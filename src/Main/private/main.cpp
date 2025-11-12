#include "EmptyScene.h"
#include "Systems/Application.h"

int main(int, char**)
{
    BixEngine::Core::Application app;
    
    if (app.Initialize())
    {
        app.EmplaceScene<BixEngine::Game::EmptyScene>();
        app.Run();
    }

    return 0;
}
