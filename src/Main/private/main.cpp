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
        
        if (!app.HasActiveScene())
        {
            LOG_INFO("Main: No default scene loaded, creating EmptyScene.");
            app.EmplaceScene<BixEngine::Game::EmptyScene>();
        }
        else
        {
            LOG_INFO("Main: Default scene already loaded. Skipping EmptyScene creation.");
        }

        app.Run();
        LOG_INFO("Main: Run loop finished.");
    }

    LOG_INFO("Main: Exiting.");
    return 0;
}
