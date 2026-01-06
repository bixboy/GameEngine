#include "Ressources/RessourcesClass/AudioClip.h"
#include <fstream>
#include "Debug/Logger.h"

namespace BixEngine::Resources
{
    bool AudioClip::LoadFromFile(const String& path)
    {
        path_ = path;

        std::ifstream file(path.c_str(), std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            LOG_ERROR("AudioClip: Failed to open file: " + path);
            return false;
        }

        std::streamsize size = file.tellg();
        if (size <= 0)
        {
            LOG_ERROR("AudioClip: File is empty or invalid: " + path);
            return false;
        }

        file.seekg(0, std::ios::beg);

        try
        {
            data_.resize(static_cast<size_t>(size));
            
            if (!file.read(reinterpret_cast<char*>(data_.data()), size))
            {
                LOG_ERROR("AudioClip: Failed to read content of: " + path);
                return false;
            }
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("AudioClip: Exception during loading: " + String(e.what()));
            return false;
        }

        LOG_INFO("AudioClip loaded: " + path + " (" + String(std::to_string(size)) + " bytes)");
        return true;
    }
}