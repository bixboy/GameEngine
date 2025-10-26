#include "FileUtils.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace BixTool
{
    bool WriteFileIfDifferent(const std::filesystem::path& path, const std::string& content)
    {
        std::string data = content;
        if (!data.empty() && data.back() != '\n')
            data.push_back('\n');

        std::string current;
        std::ifstream existing(path);
        if (existing)
        {
            std::stringstream buffer;
            buffer << existing.rdbuf();
            current = buffer.str();
            if (!current.empty() && current.back() != '\n')
                current.push_back('\n');
            if (current == data)
                return false;
        }

        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);

        std::ofstream output(path, std::ios::trunc);
        if (!output)
        {
            std::cerr << "[BixHeaderTool] Failed to write " << path << "\n";
            return false;
        }

        output << data;
        return true;
    }

    std::vector<std::filesystem::path> CollectHeaderFiles(const std::vector<std::filesystem::path>& roots)
    {
        std::vector<std::filesystem::path> files;

        auto isGenerated = [](const std::filesystem::path& p) {
            std::string name = p.filename().string();
            return name.ends_with(".generated.h");
        };

        auto isHeader = [](const std::filesystem::path& p) {
            std::string ext = p.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            return ext == ".h" || ext == ".hpp" || ext == ".hh" || ext == ".hxx";
        };

        for (const auto& root : roots)
        {
            if (!std::filesystem::exists(root))
                continue;

            if (std::filesystem::is_regular_file(root))
            {
                if (!isGenerated(root) && isHeader(root))
                    files.push_back(std::filesystem::absolute(root));
                continue;
            }

            for (const auto& entry : std::filesystem::recursive_directory_iterator(root))
            {
                if (!entry.is_regular_file())
                    continue;
                const auto& path = entry.path();
                if (!isGenerated(path) && isHeader(path))
                    files.push_back(std::filesystem::absolute(path));
            }
        }

        std::sort(files.begin(), files.end());
        files.erase(std::unique(files.begin(), files.end()), files.end());
        return files;
    }
}
