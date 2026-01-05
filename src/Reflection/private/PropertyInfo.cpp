#include "Core/PropertyInfo.h"

namespace BixEngine::Reflection
{
    bool PropertyInfo::IsValid() const noexcept
    {
        return static_cast<bool>(Access) && static_cast<bool>(ConstAccess);
    }

    void* PropertyInfo::GetRaw(void* instance) const
    {
        return Access ? Access(instance) : nullptr;
    }

    const void* PropertyInfo::GetRaw(const void* instance) const
    {
        return ConstAccess ? ConstAccess(instance) : nullptr;
    }

    bool PropertyInfo::HasMetadata(const std::string& key) const
    {
        return ParsedMetadata.find(key) != ParsedMetadata.end();
    }

    std::string PropertyInfo::GetMetadata(const std::string& key) const
    {
        auto it = ParsedMetadata.find(key);
        return it != ParsedMetadata.end() ? it->second : "";
    }

    
    static std::string Trim(const std::string& s)
    {
        auto start = s.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        auto end = s.find_last_not_of(" \t\n\r");
        return s.substr(start, end - start + 1);
    }

    void PropertyInfo::ParseMetadata()
    {
        ParsedMetadata.clear();
        if (Metadata.empty()) return;

        std::string current = Metadata;
        while (!current.empty())
        {
            size_t commaPos = current.find(',');
            std::string token = (commaPos == std::string::npos) ? current : current.substr(0, commaPos);
            
            
            size_t eqPos = token.find('=');
            if (eqPos != std::string::npos)
            {
                std::string key = Trim(token.substr(0, eqPos));
                std::string value = Trim(token.substr(eqPos + 1));
                
                
                if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
                    value = value.substr(1, value.size() - 2);
                    
                ParsedMetadata[key] = value;
            }
            else
            {
                ParsedMetadata[Trim(token)] = "";
            }

            if (commaPos == std::string::npos) 
                break;
            current = current.substr(commaPos + 1);
        }
    }
}
