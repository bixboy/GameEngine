#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>

#include "Engine/Ressources/AtlasGenerator.h"

namespace
{
    void PrintUsage()
    {
        std::cout << "Usage: BixAssetTool --generate-atlas <folder> [options]\n";
        std::cout << "Options:\n";
        std::cout << "  --columns <value>      Number of columns (auto if omitted)\n";
        std::cout << "  --rows <value>         Number of rows (auto if omitted)\n";
        std::cout << "  --padding <value>      Padding between frames\n";
        std::cout << "  --margin <value>       Margin around the atlas\n";
        std::cout << "  --frame-rate <value>   Frame rate for the generated animation\n";
        std::cout << "  --loop <true|false>    Whether the animation should loop\n";
    }

    std::unordered_map<std::string, std::string> ParseOptions(int argc, char** argv, int startIndex)
    {
        std::unordered_map<std::string, std::string> options;
        for (int i = startIndex; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg.rfind("--", 0) != 0)
                continue;

            const auto equals = arg.find('=');
            std::string key;
            std::string value;
            if (equals != std::string::npos)
            {
                key = arg.substr(2, equals - 2);
                value = arg.substr(equals + 1);
            }
            else
            {
                key = arg.substr(2);
                if (i + 1 < argc)
                {
                    value = argv[++i];
                }
            }

            if (!key.empty())
                options[key] = value;
        }

        return options;
    }

    bool StringToBool(const std::string& value)
    {
        if (value == "0" || value == "false" || value == "False" || value == "FALSE")
            return false;
        return true;
    }
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        PrintUsage();
        return EXIT_FAILURE;
    }

    const std::string command = argv[1];
    if (command != "--generate-atlas")
    {
        PrintUsage();
        return EXIT_FAILURE;
    }

    if (argc < 3)
    {
        std::cerr << "Error: missing folder argument for --generate-atlas." << std::endl;
        return EXIT_FAILURE;
    }

    const std::filesystem::path folder = argv[2];
    const auto options = ParseOptions(argc, argv, 3);

    int columns = -1;
    int rows = -1;
    int padding = 0;
    int margin = 0;
    float frameRate = 8.0f;
    bool loop = true;

    if (auto it = options.find("columns"); it != options.end())
        columns = std::stoi(it->second);
    if (auto it = options.find("rows"); it != options.end())
        rows = std::stoi(it->second);
    if (auto it = options.find("padding"); it != options.end())
        padding = std::stoi(it->second);
    if (auto it = options.find("margin"); it != options.end())
        margin = std::stoi(it->second);
    if (auto it = options.find("frame-rate"); it != options.end())
        frameRate = std::stof(it->second);
    if (auto it = options.find("loop"); it != options.end())
        loop = StringToBool(it->second);

    const bool success = BixEngine::resources::AtlasGenerator::GenerateAtlas(folder, columns, rows, padding, margin, frameRate, loop);
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
