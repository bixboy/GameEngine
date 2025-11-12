#pragma once
#include "Core/Containers/String.h"

namespace BixEngine::resources
{
    class IResource
    {
    public:
        virtual ~IResource() = default;
        virtual bool LoadFromFile(const String& path) = 0;
    };
}
