#pragma once
#include "Ressources/IResource.h"
#include <nlohmann/json.hpp>

namespace BixEngine::resources
{
    class ComponentPrefab : public IResource
    {
    public:
        bool LoadFromFile(const String& path) override;
        
        [[nodiscard]] const nlohmann::json& GetData() const noexcept { return data_; }

    private:
        nlohmann::json data_;
    };
}
