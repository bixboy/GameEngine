#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserActions.h"

#include "Core/Logger.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserFileUtils.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserState.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentEntry.h"

#include "imgui.h"

#include <cstdio>

namespace BixEngine::Gui
{
    namespace
    {
        // ─────────────────────────────────────────────
        // 🔧  Helpers internes pour les actions
        // ─────────────────────────────────────────────

        EntryAction MakeSeparator()
        {
            return EntryAction{nullptr, {}, true};
        }

        void RequestFolderCreation(PopupRequestState& requests, const std::filesystem::path& target)
        {
            std::snprintf(requests.folderName, IM_ARRAYSIZE(requests.folderName), "%s", "NewFolder");
            requests.folderError.Clear();
            requests.folderTarget = target;
            requests.createFolder = true;
        }

        void RequestScriptCreation(PopupRequestState& requests)
        {
            std::snprintf(requests.scriptName, IM_ARRAYSIZE(requests.scriptName), "%s", "NewScript");
            requests.scriptType = ScriptTemplateType::Actor;
            requests.scriptError.Clear();
            requests.createScript = true;
            ClearSelectedParent(requests);
        }

        bool DeleteScriptFiles(const ContentEntry& entry, String& error)
        {
            const auto removeFile = [&](const std::filesystem::path& path)
            {
                if (path.empty())
                    return true;

                return TryRemove(path, false, error);
            };

            const bool headerResult = removeFile(entry.headerPath);
            const bool sourceResult = removeFile(entry.sourcePath);
            if (!headerResult || !sourceResult)
            {
                return false;
            }

            return true;
        }
    }

    // ─────────────────────────────────────────────
    // 🛠️  Actions contextuelles du Content Browser
    // ─────────────────────────────────────────────

    EntryActionList BuildActionsFor(const ContentBrowserState& state, const ContentEntry& entry)
    {
        EntryActionList actions{};

        if (entry.IsDirectory())
        {
            actions.push_back({
                "Open",
                [](ContentBrowserState& contentState, const ContentEntry& directory, PopupRequestState&, String& selection)
                {
                    contentState.current = directory.path;
                    selection.Clear();
                }
            });

            actions.push_back({
                "Create script...",
                [](ContentBrowserState&, const ContentEntry&, PopupRequestState& requests, String&)
                {
                    RequestScriptCreation(requests);
                }
            });

            actions.push_back({
                "Create folder...",
                [](ContentBrowserState&, const ContentEntry& directory, PopupRequestState& requests, String&)
                {
                    RequestFolderCreation(requests, directory.path);
                }
            });
        }
        else if (entry.IsScript())
        {
            const bool hasHeader = entry.HasHeader();
            const bool hasSource = entry.HasSource();

            actions.push_back({
                "Open header",
                [hasHeader](ContentBrowserState& contentState, const ContentEntry& script, PopupRequestState&, String& selection)
                {
                    if (!hasHeader)
                        return;

                    selection = script.SelectionKey();
                    if (contentState.openScriptFilesCallback)
                        contentState.openScriptFilesCallback({script.headerPath});
                },
                hasHeader
            });

            actions.push_back({
                "Open source",
                [hasSource](ContentBrowserState& contentState, const ContentEntry& script, PopupRequestState&, String& selection)
                {
                    if (!hasSource)
                        return;

                    selection = script.SelectionKey();
                    if (contentState.openScriptFilesCallback)
                        contentState.openScriptFilesCallback({script.sourcePath});
                },
                hasSource
            });

            actions.push_back({
                "Open both",
                [hasHeader, hasSource](ContentBrowserState& contentState, const ContentEntry& script, PopupRequestState&, String& selection)
                {
                    if (!hasHeader && !hasSource)
                        return;

                    selection = script.SelectionKey();
                    if (contentState.openScriptFilesCallback)
                    {
                        std::vector<std::filesystem::path> files{};
                        if (hasHeader)
                            files.push_back(script.headerPath);
                        if (hasSource)
                            files.push_back(script.sourcePath);
                        contentState.openScriptFilesCallback(files);
                    }
                },
                hasHeader || hasSource
            });
        }
        else if (entry.IsActor())
        {
            actions.push_back({
                "Open actor editor",
                [](ContentBrowserState& contentState, const ContentEntry& actor, PopupRequestState&, String& selection)
                {
                    selection = actor.SelectionKey();
                    if (contentState.openActorEditorCallback)
                        contentState.openActorEditorCallback(actor.path);
                },
                static_cast<bool>(state.openActorEditorCallback)
            });
        }
        else
        {
            actions.push_back({
                "Open",
                [](ContentBrowserState&, const ContentEntry& file, PopupRequestState&, String& selection)
                {
                    selection = file.SelectionKey();
                }
            });
        }

        if (!actions.empty())
            actions.push_back(MakeSeparator());

        actions.push_back({
            "Rename...",
            [](ContentBrowserState&, const ContentEntry& entry, PopupRequestState& requests, String&)
            {
                const String entryName = entry.IsScript() ? entry.name : String(entry.path.filename().generic_string());
                std::snprintf(requests.renameBuffer, IM_ARRAYSIZE(requests.renameBuffer), "%s", entryName.c_str());
                requests.renameError.Clear();
                requests.renameTarget = entry.IsScript() ? entry.headerPath : entry.path;
                requests.renameSecondaryTarget = entry.IsScript() ? entry.sourcePath : std::filesystem::path{};
                requests.renameTargetIsScriptGroup = entry.IsScript();
                requests.renameEntry = true;
            }
        });

        actions.push_back({
            "Delete",
            [](ContentBrowserState& contentState, const ContentEntry& entry, PopupRequestState&, String& selection)
            {
                String error;
                bool success = true;

                if (entry.IsDirectory())
                {
                    success = TryRemove(entry.path, true, error);
                }
                else if (entry.IsScript())
                {
                    success = DeleteScriptFiles(entry, error);
                }
                else
                {
                    success = TryRemove(entry.path, false, error);
                }

                if (success)
                {
                    contentState.cache.dirty = true;
                    if (selection == entry.SelectionKey())
                        selection.Clear();

                    if (entry.IsDirectory() && contentState.current == entry.path)
                        contentState.current = contentState.root;
                }
                else if (!error.IsEmpty())
                {
                    LOG_ERROR(error);
                }
            }
        });

        actions.push_back(MakeSeparator());

        actions.push_back({
            "Reveal in Explorer",
            [](ContentBrowserState&, const ContentEntry& entry, PopupRequestState&, String&)
            {
                const std::filesystem::path explorerPath = entry.IsScript()
                    ? (!entry.headerPath.empty() ? entry.headerPath : entry.sourcePath)
                    : entry.path;
                ShowPathInExplorer(explorerPath, entry.IsDirectory());
            }
        });

        actions.push_back({
            "New folder here...",
            [](ContentBrowserState&, const ContentEntry& entry, PopupRequestState& requests, String&)
            {
                const std::filesystem::path folderTarget = entry.IsDirectory()
                    ? entry.path
                    : entry.path.parent_path();
                RequestFolderCreation(requests, folderTarget);
            }
        });

        if (!entry.IsDirectory())
        {
            actions.push_back({
                "Create script...",
                [](ContentBrowserState&, const ContentEntry&, PopupRequestState& requests, String&)
                {
                    RequestScriptCreation(requests);
                }
            });
        }

        return actions;
    }

    void DrawEntryContextMenu(ContentBrowserState& state, const ContentEntry& entry, PopupRequestState& requests, String& selectionKey)
    {
        const EntryActionList actions = BuildActionsFor(state, entry);
        for (const EntryAction& action : actions)
        {
            if (!action.label)
            {
                ImGui::Separator();
                continue;
            }

            const bool enabled = action.enabled && static_cast<bool>(action.callback);
            if (ImGui::MenuItem(action.label, nullptr, false, enabled))
                action.callback(state, entry, requests, selectionKey);
        }
    }
}

