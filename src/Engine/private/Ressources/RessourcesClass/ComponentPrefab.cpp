#include "Ressources/RessourcesClass/ComponentPrefab.h"
#include <fstream>
#include "Logger.h"

namespace BixEngine::resources
{
    bool ComponentPrefab::LoadFromFile(const String& path)
    {
        std::ifstream file(path.Std());
        if (!file.is_open())
        {
            LOG_ERROR("Failed to open component prefab file: " + path);
            return false;
        }

        try
        {
            data_ = nlohmann::json::parse(file);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to parse component prefab JSON: " + String(e.what()));
            return false;
        }

        return true;
    }
}
