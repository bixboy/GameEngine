#include "Utils/FileIO/FilesUtils.h"
#include "Debug/Logger.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <imgui.h>

namespace BixEngine::Utils
{
    // --- Gestion Dossiers ---

    bool FileUtils::TryCreateDir(const fs::path& dir, String& outError)
    {
        std::error_code ec;
        if (dir.empty())
        {
            return LogAndStoreError(outError, "Invalid directory path.");
        }

        if (fs::exists(dir))
            return true;

        fs::create_directories(dir, ec);
        if (ec)
        {
            return LogAndStoreError(outError, String("Failed to create directory: ") + dir.string() + " (" + ec.message() + ")");
        }

        return true;
    }

    bool FileUtils::EnsureDirectory(const fs::path& dir)
    {
        String dummyError;
        return TryCreateDir(dir, dummyError);
    }

    bool FileUtils::EnsureParentDirectory(const fs::path& filePath)
    {
        if (filePath.empty())
            return false;
        
        return EnsureDirectory(filePath.parent_path());
    }

    // --- Manipulation ---

    bool FileUtils::TryRemove(const fs::path& path, bool recursive, String& outError)
    {
        std::error_code ec;
        if (recursive)
        {
            fs::remove_all(path, ec);
        }
        else
        {
            fs::remove(path, ec);
        }

        if (ec)
        {
            return LogAndStoreError(outError, String("Failed to remove: ") + path.string() + " (" + ec.message() + ")");
        }
        
        return true;
    }

    bool FileUtils::TryCopyFile(const fs::path& source, const fs::path& destination, bool overwrite, String& outError)
    {
        std::error_code ec;

        if (!fs::exists(source))
        {
            return LogAndStoreError(outError, String("Source file not found: ") + source.string());
        }

        if (destination.has_parent_path())
        {
            if (!EnsureDirectory(destination.parent_path()))
            {
                 return LogAndStoreError(outError, "Failed to create parent directory for copy.");
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
            return LogAndStoreError(outError, String("Failed to copy file: ") + source.string() + " -> " + destination.string());
        }

        return true;
    }

    bool FileUtils::TryRename(const fs::path& source, const fs::path& destination, bool overwrite, String& outError)
    {
        std::error_code ec;

        if (source.empty() || destination.empty())
        {
            return LogAndStoreError(outError, "Invalid source or destination path.");
        }

        if (!fs::exists(source))
        {
            return LogAndStoreError(outError, String("Source not found: ") + source.string());
        }

        if (fs::exists(destination))
        {
            if (!overwrite)
            {
                return LogAndStoreError(outError, String("Destination already exists: ") + destination.string());
            }
            
            fs::remove(destination, ec); 
        }

        if (destination.has_parent_path())
        {
            EnsureDirectory(destination.parent_path());
        }

        fs::rename(source, destination, ec);
        if (ec)
        {
            return LogAndStoreError(outError, String("Failed to rename: ") + ec.message());
        }

        return true;
    }

    // --- Lecture / Écriture ---

    bool FileUtils::TryWriteFile(const fs::path& path, const std::string& content, String& outError)
    {
        if (path.empty())
            return LogAndStoreError(outError, "Invalid file path.");

        EnsureParentDirectory(path);

        std::ofstream file(path, std::ios::out | std::ios::trunc);
        if (!file.is_open())
        {
            return LogAndStoreError(outError, String("Failed to open file for writing: ") + path.string());
        }

        file << content;
        file.flush();
        file.close();

        return true;
    }

    bool FileUtils::ReadFile(const fs::path& path, String& outContent)
    {
        std::ifstream file(path);
        if (!file.is_open())
            return false;

        std::stringstream buffer;
        buffer << file.rdbuf();
        outContent = buffer.str().c_str();
        return true;
    }

    // --- Scan ---

    std::vector<fs::path> FileUtils::ScanDirectory(const fs::path& directory, const std::vector<std::string>& extensions, bool recursive)
    {
        std::vector<fs::path> results;
        fs::path finalPath = directory;

        if (!fs::exists(finalPath))
        {
            fs::path contentPath = "Content" / finalPath;
            if (fs::exists(contentPath))
                finalPath = contentPath;
            
            else return results; 
        }

        std::vector<std::string> lowerExts;
        lowerExts.reserve(extensions.size());
        for (const auto& ext : extensions)
        {
            std::string lower = ext;
            std::ranges::transform(lower, lower.begin(),
               [](unsigned char c)
               {
                   return std::tolower(c);
               });
            
            lowerExts.push_back(lower);
        }

        auto ProcessEntry = [&](const fs::directory_entry& entry)
        {
            if (!entry.is_regular_file())
                return;

            if (lowerExts.empty())
            {
                results.push_back(entry.path());
                return;
            }

            std::string fileExt = entry.path().extension().string();
            std::ranges::transform(fileExt, fileExt.begin(),
               [](unsigned char c)
               {
                   return std::tolower(c);
               });
            
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
                for (const auto& entry : fs::recursive_directory_iterator(finalPath))
                    ProcessEntry(entry);
            }
            else
            {
                for (const auto& entry : fs::directory_iterator(finalPath))
                    ProcessEntry(entry);
            }
        }
        catch (const fs::filesystem_error& e)
        {
            LOG_ERROR("Error scanning directory: " + String(e.what()));
        }

        return results;
    }

    // --- Utilitaires ---

    bool FileUtils::IsExtension(const fs::path& path, const std::string& extension)
    {
        if (!path.has_extension())
            return false;
        
        std::string ext = path.extension().string();
        
        return CaseInsensitiveLess(ext, extension) || CaseInsensitiveLess(extension, ext) || ext == extension;
    }

    bool FileUtils::LogAndStoreError(String& outError, const String& message, bool alsoLog)
    {
        outError = message;
        if (alsoLog)
            LOG_ERROR(message);
        
        return false;
    }

    bool FileUtils::CaseInsensitiveLess(const std::string& a, const std::string& b)
    {
        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(),
            [](unsigned char x, unsigned char y)
            {
                return std::tolower(x) < std::tolower(y);
            });
    }

    String FileUtils::ExtractDisplayName(const fs::path& path)
    {
        if (path.empty())
            return "Asset";
        
        if (path.has_stem())
            return String(path.stem().generic_string().c_str());
        
        return String(path.generic_string().c_str());
    }

    fs::path FileUtils::NormalizePath(const fs::path& path)
    {
        if (path.empty())
            return {};
        
        return path.lexically_normal().make_preferred();
    }

    fs::path FileUtils::ResolveUserConfigPath(const char* fileName)
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