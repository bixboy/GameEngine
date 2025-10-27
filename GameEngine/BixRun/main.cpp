#include "Engine/Systems/BixEngine.h"

int main(int, char**)
{
    BixEngine::Application app;
    
    if (app.Initialize())
    {
        app.EmplaceScene<BixEngine::EmptyScene>();
        app.Run();
    }

    return 0;
}
