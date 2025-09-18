#include "Engine.h"

int main(int, char**)
{
    Engine::Application app;
    if (app.Initialize())
    {
        app.EmplaceScene<Engine::TestScene>();
        app.Run();
    }

    return 0;
}
