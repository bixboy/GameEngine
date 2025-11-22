#include "Ressources/RessourcesClass/AudioClip.h"
#include <fstream>
#include "Logger.h"


namespace BixEngine::resources
{
    bool AudioClip::LoadFromFile(const String& path)
    {
        path_ = path;
        std::ifstream file(path.c_str(), std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            LOG_ERROR("Failed to open audio file: " + path);
            return false;
        }

        std::streamsize size = file.tellg();
        if (size <= 0)
        {
            LOG_ERROR("Audio file is empty or invalid: " + path);
            return false;
        }

        file.seekg(0, std::ios::beg);

        data_.resize(static_cast<size_t>(size));
        if (!file.read(reinterpret_cast<char*>(data_.data()), size))
        {
            LOG_ERROR("Failed to read audio file: " + path);
            return false;
        }

        return true;
    }
}
