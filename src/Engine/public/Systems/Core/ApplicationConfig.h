#pragma once
#include "Math/Color.h"
#include "Containers/String.h"


namespace BixEngine::Core
{
    struct ApplicationConfig
    {
        String windowTitle{"BixEngine"};

        int width{1280};

        int height{720};

        bool resizable{true};

        Math::Color clearColor{0, 0, 0, 255};
    };
}
