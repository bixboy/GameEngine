#pragma once
#include "Core/Containers/String.h"

namespace BixEngine::Core
{
    class IResource
    {
    public:
        virtual ~IResource() = default;
        virtual bool LoadFromFile(const String& path) = 0;
    };
}
