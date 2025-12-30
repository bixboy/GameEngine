#pragma once
#include "Containers/String.h"

namespace BixEngine::Resources
{
    class IResource
    {
    public:
        virtual ~IResource() = default;
        
        virtual bool LoadFromFile(const String& path) 
        { 
            (void)path; 
            return false; 
        }
    };
}