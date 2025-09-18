#include "Engine.h"

int main(int, char**) {
    Engine::Core::Application app;
    if (app.Initialize()) {
        app.EmplaceScene<Engine::Game::EmptyScene>();
        app.Run();
    }

    return 0;
}
