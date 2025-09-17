#include "../Engine/Public/Core/Application.h"

int main(int, char**) {
    
    Application app;
    if (app.Init())
        app.Run();
    
    return 0;
}