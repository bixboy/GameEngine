#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserState.h"

#include "Core/Logger.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserFileUtils.h"

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

    // ─────────────────────────────────────────────
    // 🧹  Réinitialisation des sélections de parent
    // ─────────────────────────────────────────────

    void ClearSelectedParent(PopupRequestState& requests)
    {
        requests.selectedParentClass.Clear();
        requests.selectedParentInclude.Clear();
        requests.selectedParentDisplay.Clear();
        requests.selectedParentIsBase = false;
        requests.selectedParentIsActor = false;
        requests.selectedParentIsComponent = false;
    }

    // ─────────────────────────────────────────────
    // 🚀  Initialisation du Content Browser
    // ─────────────────────────────────────────────

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
        else if (!TryCreateDir(state.root, state.error))
        {
            // Error already stored.
        }
        else if (!fs::exists(state.root))
        {
            String message = String("Content directory is not available: ") + state.root.string();
            LogAndStoreError(state.error, std::move(message));
        }
        else
        {
            state.current = state.root;
            state.cache.directory.clear();
            state.cache.entries.clear();
            state.cache.dirty = true;
            state.error.Clear();
        }

        state.initialized = true;
        return state.error.IsEmpty();
    }

    // ─────────────────────────────────────────────
    // 🧱  Vérification du dossier Scripts
    // ─────────────────────────────────────────────

    void EnsureScriptsDirectoryExists(const ContentBrowserState& state)
    {
        if (state.root.empty())
            return;

        const std::filesystem::path scriptsDirectory = state.root / "Scripts";
        String errorStorage;
        if (!TryCreateDir(scriptsDirectory, errorStorage))
        {
            String message = String("Failed to create scripts directory: ") + scriptsDirectory.string();
            message += " (";
            message += errorStorage;
            message += ')';
            LOG_ERROR(message);
        }
    }
}

