#include "ContentBrowserPanelInternal.h"

#include "Bix/Core/Logger.h"

#include <filesystem>
#include <system_error>

namespace BixEngine::Gui
{
    namespace
    {
        std::filesystem::path GetContentRoot()
        {
            namespace fs = std::filesystem;

            std::error_code cwdError;
            const fs::path basePath = fs::current_path(cwdError);
            if (cwdError)
            {
                String message = String("Failed to determine working directory: ");
                message += cwdError.message();
                LOG_ERROR(message);
                return {};
            }

            return basePath / "Content";
        }
    }

    bool EnsureContentBrowserInitialized(ContentBrowserState& state)
    {
        namespace fs = std::filesystem;

        if (state.initialized)
            return state.error.IsEmpty();

        state.root = GetContentRoot();
        if (state.root.empty())
        {
            LogAndStoreError(state.error, "Unable to determine the Content directory root.");
        }
        else
        {
            std::error_code createError;
            fs::create_directories(state.root, createError);
            if (createError)
            {
                String message = String("Failed to create content directory: ") + state.root.string();
                message += " (";
                message += createError.message();
                message += ')';
                LogAndStoreError(state.error, std::move(message));
            }
            else if (!fs::exists(state.root))
            {
                String message = String("Content directory is not available: ") + state.root.string();
                LogAndStoreError(state.error, std::move(message));
            }
            else
            {
                state.current = state.root;
            }
        }

        state.initialized = true;
        return state.error.IsEmpty();
    }

    void EnsureScriptsDirectoryExists(const ContentBrowserState& state)
    {
        namespace fs = std::filesystem;

        if (state.root.empty())
            return;

        const fs::path scriptsDirectory = state.root / "Scripts";
        std::error_code createError;
        fs::create_directories(scriptsDirectory, createError);
        if (createError)
        {
            String message = String("Failed to create scripts directory: ") + scriptsDirectory.string();
            message += " (";
            message += createError.message();
            message += ')';
            LOG_ERROR(message);
        }
    }
}
