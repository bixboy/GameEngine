#pragma once
#include "Ressources/IResource.h"
#include <vector>

namespace BixEngine::resources
{
    class AudioClip : public IResource
    {
    public:
        bool LoadFromFile(const String& path) override;
        
        const std::vector<uint8_t>& GetData() const { return data_; }
        const String& GetPath() const { return path_; }

    private:
        std::vector<uint8_t> data_;
        String path_;
    };
}
