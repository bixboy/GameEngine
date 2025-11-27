#include "Levels/EmptyScene.h"
#include "Systems/Core/Application.h"
#include "Debug/Logger.h"

int main(int, char**)
{
    LOG_INFO("Main: Starting application");
    BixEngine::Core::Application app;
    
    if (app.Initialize())
    {
        LOG_INFO("Main: Initialized. Starting Run loop.");
        app.EmplaceScene<BixEngine::Game::EmptyScene>();
        app.Run();
        LOG_INFO("Main: Run loop finished.");
    }

    LOG_INFO("Main: Exiting.");
    return 0;
}
