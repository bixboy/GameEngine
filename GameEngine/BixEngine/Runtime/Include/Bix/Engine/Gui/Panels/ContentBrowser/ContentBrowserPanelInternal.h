#pragma once

#include "Bix/Core/String.h"

#include "imgui.h"

#include <filesystem>
#include <functional>
#include <vector>

namespace BixEngine::Gui
{
    struct ContentBrowserState
    {
        std::filesystem::path root{};
        std::filesystem::path current{};
        String error{};
        bool initialized{false};
        std::function<void(const std::vector<std::filesystem::path>&)> openScriptFilesCallback{};
    };

    enum class ScriptTemplateType
    {
        Actor = 0,
        Component,
        Utility,
    };

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
        std::vector<String> existingScripts{};
        int selectedParentScript{-1};
    };

    inline constexpr ImVec4 kContentBackground{0.09f, 0.09f, 0.09f, 0.95f};
    inline constexpr ImVec4 kContentTreeBackground{0.13f, 0.13f, 0.13f, 0.95f};
    inline constexpr ImVec4 kContentHeaderBackground{0.16f, 0.16f, 0.16f, 1.0f};
    inline constexpr float kContentTreeWidth = 240.0f;
    inline constexpr float kContentHeaderHeight = 72.0f;
    inline constexpr float kContentThumbnailSize = 72.0f;
    inline constexpr float kContentThumbnailPadding = 28.0f;

#ifdef ImGuiHoveredFlags_ForTooltip
    inline constexpr ImGuiHoveredFlags kEntryTooltipHoverFlags = ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_ForTooltip;
#else
    inline constexpr ImGuiHoveredFlags kEntryTooltipHoverFlags = ImGuiHoveredFlags_DelayNormal;
#endif

    void LogAndStoreError(String& storage, String message, bool log = true);
    String ToLowerCopy(const String& value);
    bool CaseInsensitiveLess(const String& lhs, const String& rhs);
    String TrimCopy(String value);
    bool ContainsPathSeparator(const String& value);
    void ShowPathInExplorer(const std::filesystem::path& path, bool isDirectory);
    bool MatchesSearch(const String& value, const String& query);

    bool EnsureContentBrowserInitialized(ContentBrowserState& state);
    void EnsureScriptsDirectoryExists(const ContentBrowserState& state);

    void RenderHeader(ContentBrowserState& state, String& selectedEntry, char (&searchBuffer)[256]);
    void RenderDirectoryTree(ContentBrowserState& state, String& selectedEntry);
    void RenderEntries(ContentBrowserState& state, String& selectedEntry, PopupRequestState& requestPopups, const String& searchQuery);

    void RenderPopups(ContentBrowserState& state, String& selectedEntry, PopupRequestState& requestPopups);
}
