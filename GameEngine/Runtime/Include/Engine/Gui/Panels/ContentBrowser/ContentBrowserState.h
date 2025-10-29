#pragma once

#include "Core/Containers/String.h"

#include <filesystem>
#include <functional>
#include <vector>

namespace BixEngine::Gui
{
    enum class ScriptTemplateType
    {
        Actor = 0,
        Component,
        Utility,
    };

    struct ContentEntry;

    struct PopupRequestState
    {
        bool createScript{false};
        bool createFolder{false};
        bool renameEntry{false};

        ScriptTemplateType scriptType{ScriptTemplateType::Actor};
        char scriptName[128] = "NewScript";
        char folderName[128] = "NewFolder";
        char renameBuffer[256] = "";
        String scriptError{};
        String folderError{};
        String renameError{};
        std::filesystem::path folderTarget{};
        std::filesystem::path renameTarget{};
        std::filesystem::path renameSecondaryTarget{};
        bool renameTargetIsScriptGroup{false};
        String selectedParentClass{};
        String selectedParentInclude{};
        String selectedParentDisplay{};
        bool selectedParentIsBase{false};
        bool selectedParentIsActor{false};
        bool selectedParentIsComponent{false};
    };

    struct DirectoryCache
    {
        std::filesystem::path directory{};
        std::vector<ContentEntry> entries{};
        bool dirty{true};
    };

    struct ContentBrowserState
    {
        std::filesystem::path root{};
        std::filesystem::path current{};
        String error{};
        bool initialized{false};
        std::function<void(const std::vector<std::filesystem::path>&)> openScriptFilesCallback{};
        std::function<void(const std::filesystem::path&)> openActorEditorCallback{};
        DirectoryCache cache{};
    };

    void ClearSelectedParent(PopupRequestState& requests);

    bool EnsureContentBrowserInitialized(ContentBrowserState& state);
    void EnsureScriptsDirectoryExists(const ContentBrowserState& state);
}

