#include "Gui/Utils/ExposedVariableUtils.h"
#include <cctype>
#include <string>


namespace BixEngine::Gui
{
    bool ExposedVariableUtils::MatchesType(std::string_view typeName, std::string_view expectedSuffix)
    {
        if (typeName.size() < expectedSuffix.size())
            return false;

        size_t offset = typeName.size() - expectedSuffix.size();
        if (typeName.compare(offset, expectedSuffix.size(), expectedSuffix) != 0)
            return false;

        if (offset == 0)
            return true;

        char preceding = typeName[offset - 1];
        return !std::isalnum((unsigned char)preceding) && preceding != '_';
    }

    std::string ExposedVariableUtils::TrimBraces(std::string_view input)
    {
        auto first = input.find_first_not_of(" \t{}\n\r");
        auto last  = input.find_last_not_of(" \t{}\n\r");

        if (first == std::string_view::npos)
            return {};

        return std::string(input.substr(first, last - first + 1));
    }

    std::vector<float> ExposedVariableUtils::ExtractNumbers(std::string_view input)
    {
        std::vector<float> numbers;
        std::string current;

        auto flush = [&]() {
            if (!current.empty())
            {
                try
                {
                    numbers.push_back(std::stof(current));
                }
                catch (...) {}
                
                current.clear();
            }
        };

        for (char c : input)
        {
            if (std::isdigit((unsigned char)c) || c=='+' || c=='-' || c=='.' || c=='e' || c=='E')
                current.push_back(c);
            else
                flush();
        }

        flush();
        return numbers;
    }

    std::string ExposedVariableUtils::CleanTypeName(std::string typeName)
    {
        if (typeName.rfind("class ", 0) == 0)
            typeName.erase(0, 6);
        
        if (typeName.rfind("struct ", 0) == 0)
            typeName.erase(0, 7);
        
        return typeName;
    }

    std::string ExposedVariableUtils::MakeDisplayName(const std::string& rawName)
    {
        std::string trimmed = rawName;
        
        while (!trimmed.empty() && trimmed.back() == '_')
            trimmed.pop_back();

        if (trimmed.empty())
            return "Property";

        std::string result;
        char prev = '\0';

        for (char c : trimmed)
        {
            if (c == '_')
            {
                if (!result.empty() && result.back() != ' ')
                    result.push_back(' ');
                
                prev = c;
                continue;
            }

            bool isUpper = std::isupper((unsigned char)c);
            bool prevLower = std::islower((unsigned char)prev);

            if (!result.empty() && isUpper && prevLower)
                result.push_back(' ');

            result.push_back(c);
            prev = c;
        }

        if (!result.empty())
            result[0] = std::toupper(result[0]);

        return result;
    }
}
