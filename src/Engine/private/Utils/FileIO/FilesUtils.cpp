#include "Utils/FileIO/FilesUtils.h"
#include "Debug/Logger.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <imgui.h>


namespace BixEngine::FilesUtils
{
    namespace fs = std::filesystem;

    // ─────────────────────────────────────────────
    // Gestion des dossiers
    // ─────────────────────────────────────────────
    bool Utilities::TryCreateDir(const fs::path& dir, String& outError)
    {
        std::error_code ec;
        if (dir.empty())
        {
            outError = "Invalid directory path.";
            return false;
        }

        fs::create_directories(dir, ec);
        if (ec)
        {
            outError = String("Failed to create directory: ") + dir.string() + " (" + ec.message() + ")";
            LOG_ERROR(outError);
            return false;
        }

        return true;
    }

    bool Utilities::TryRemove(const fs::path& path, bool recursive, String& outError)
    {
        std::error_code ec;
        if (recursive)
            fs::remove_all(path, ec);
        else
            fs::remove(path, ec);

        if (ec)
        {
            outError = String("Failed to remove: ") + path.string() + " (" + ec.message() + ")";
            LOG_ERROR(outError);
            return false;
        }

        return true;
    }

    // ─────────────────────────────────────────────
    // Gestion de fichiers
    // ─────────────────────────────────────────────
    bool Utilities::TryCopyFile(const fs::path& source, const fs::path& destination, bool overwrite, String& outError)
    {
        std::error_code ec;

        if (!fs::exists(source))
        {
            outError = String("Source file not found: ") + source.string();
            LOG_ERROR(outError);
            return false;
        }

        if (destination.has_parent_path())
        {
            fs::create_directories(destination.parent_path(), ec);
            if (ec)
            {
                outError = String("Failed to create destination directory: ") + destination.parent_path().string() + " (" + ec.message() + ")";
                LOG_ERROR(outError);
                return false;
            }
        }

        if (fs::exists(destination) && !overwrite)
        {
            outError = String("Destination file already exists: ") + destination.string();
            LOG_WARNING(outError);
            return false;
        }

        fs::copy_file(source, destination, overwrite ? fs::copy_options::overwrite_existing : fs::copy_options::none, ec);

        if (ec)
        {
            outError = String("Failed to copy file: ") + source.string() + " → " + destination.string() + " (" + ec.message() + ")";
            LOG_ERROR(outError);
            return false;
        }

        return true;
    }

    bool Utilities::TryRename(const fs::path& source, const fs::path& destination, bool overwrite, String& outError)
    {
        std::error_code ec;

        if (source.empty() || destination.empty())
        {
            outError = "Invalid source or destination path.";
            return false;
        }

        if (!fs::exists(source))
        {
            outError = String("Source not found: ") + source.string();
            LOG_WARNING(outError);
            return false;
        }

        if (fs::exists(destination))
        {
            if (!overwrite)
            {
                outError = String("Destination already exists: ") + destination.string();
                LOG_WARNING(outError);
                return false;
            }

            fs::remove(destination, ec);
            if (ec)
            {
                outError = String("Failed to remove existing destination before rename: ") + destination.string() + " (" + ec.message() + ")";
                LOG_ERROR(outError);
                return false;
            }
        }

        if (destination.has_parent_path())
        {
            fs::create_directories(destination.parent_path(), ec);
            if (ec)
            {
                outError = String("Failed to create destination directory: ") + destination.parent_path().string() + " (" + ec.message() + ")";
                LOG_ERROR(outError);
                return false;
            }
        }

        fs::rename(source, destination, ec);
        if (ec)
        {
            outError = String("Failed to rename: ") + source.string() + " → " + destination.string() + " (" + ec.message() + ")";
            LOG_ERROR(outError);
            return false;
        }

        return true;
    }

    bool Utilities::TryWriteFile(const fs::path& path, const std::string& content, String& outError)
    {
        std::error_code ec;

        if (path.empty())
        {
            outError = "Invalid file path.";
            return false;
        }

        if (path.has_parent_path())
        {
            fs::create_directories(path.parent_path(), ec);
            if (ec)
            {
                outError = String("Failed to create target directory: ") + path.parent_path().string() + " (" + ec.message() + ")";
                LOG_ERROR(outError);
                return false;
            }
        }

        std::ofstream file(path, std::ios::out | std::ios::trunc);
        if (!file.is_open())
        {
            outError = String("Failed to open file for writing: ") + path.string();
            LOG_ERROR(outError);
            return false;
        }

        file << content;
        file.flush();

        if (!file.good())
        {
            outError = String("Failed to write file: ") + path.string();
            LOG_ERROR(outError);
            file.close();
            return false;
        }

        file.close();
        return true;
    }

    std::vector<std::filesystem::path> Utilities::ScanDirectory(const std::filesystem::path& directory, const std::vector<std::string>& extensions, bool recursive)
    {
        std::vector<std::filesystem::path> results;
        std::filesystem::path finalPath = directory;

        if (!std::filesystem::exists(finalPath))
        {
            std::filesystem::path contentPath = "Content" / finalPath;
            if (std::filesystem::exists(contentPath))
            {
                finalPath = contentPath;
            }
            else
            {
                return results; 
            }
        }

        std::vector<std::string> lowerExts;
        lowerExts.reserve(extensions.size());
        for (const auto& ext : extensions)
        {
            lowerExts.push_back(String(ext).ToLower().Std());   
        }

        auto ProcessEntry = [&](const std::filesystem::directory_entry& entry)
        {
            if (!entry.is_regular_file())
                return;

            if (lowerExts.empty())
            {
                results.push_back(entry.path());
                return;
            }

            std::string fileExt = String(entry.path().extension().string()).ToLower();
            
            for (const auto& allowedExt : lowerExts)
            {
                if (fileExt == allowedExt)
                {
                    results.push_back(entry.path());
                    break;
                }
            }
        };
        
        try
        {
            if (recursive)
            {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(finalPath))
                    ProcessEntry(entry);
            }
            else
            {
                for (const auto& entry : std::filesystem::directory_iterator(finalPath))
                    ProcessEntry(entry);
            }
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            LOG_ERROR("Error scanning directory");
        }

        return results;
    }

    // ─────────────────────────────────────────────
    // Gestion d’erreurs et comparaison
    // ─────────────────────────────────────────────
    bool Utilities::LogAndStoreError(String& outError, const String& message, bool alsoLog)
    {
        outError = message;
        if (alsoLog)
            LOG_ERROR(message);

        return false;
    }

    bool Utilities::CaseInsensitiveLess(const std::string& a, const std::string& b)
    {
        return std::lexicographical_compare(
            a.begin(), a.end(),
            b.begin(), b.end(),
            [](unsigned char x, unsigned char y) { return std::tolower(x) < std::tolower(y); });
    }

    // ─────────────────────────────────────────────
    // Utilitaires
    // ─────────────────────────────────────────────
    String Utilities::ExtractDisplayName(const fs::path& path)
    {
        if (path.empty())
            return "Asset";

        const auto filename = path.filename().generic_string();
        if (!filename.empty())
            return String{filename};

        const auto stem = path.stem().generic_string();
        if (!stem.empty())
            return String{stem};

        return String{path.generic_string()};
    }

    fs::path Utilities::NormalizePath(const fs::path& path)
    {
        if (path.empty())
            return {};

        fs::path p = path;
        p = p.make_preferred();
        return p.lexically_normal();
    }

    fs::path Utilities::ResolveUserConfigPath(const char* fileName)
    {
        const ImGuiIO& io = ImGui::GetIO();
        fs::path base;

        if (io.IniFilename && io.IniFilename[0] != '\0')
            base = fs::path(io.IniFilename).parent_path();

        if (base.empty())
            base = fs::current_path();

        return base / fileName;
    }
}
