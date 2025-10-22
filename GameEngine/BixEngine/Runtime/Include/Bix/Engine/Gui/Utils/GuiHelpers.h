#pragma once

#include <functional>
#include <string>
#include <vector>

#include "imgui.h"

namespace BixEngine::Gui::Utils
{
    /// Represents a node in a tree hierarchy.
    struct TreeNodeData
    {
        std::string name;
        std::vector<TreeNodeData> children;
        bool isLeaf = false;
    };

    /// Callback invoked for tree nodes (e.g., context menus or actions).
    using TreeNodeCallback = std::function<void(const TreeNodeData&)>;

    /// Callback invoked when interacting with list items.
    using ListItemCallback = std::function<void(const std::string&)>;

    // --- General -----------------------------------------------------------------

    /// Draws a stylised header section delimiter.
    void DrawSectionHeader(const char* title);

    /// Displays an error message using the configured warning colours.
    void DrawErrorMessage(const std::string& message);

    /// Draws confirm and cancel buttons and returns true when the confirm button is pressed.
    bool DrawConfirmButtons(const char* okLabel, const char* cancelLabel);

    /// Draws confirm and cancel buttons while invoking optional callbacks.
    bool DrawConfirmButtons(const char* okLabel, const char* cancelLabel, const std::function<void()>& onConfirm, const std::function<void()>& onCancel);

    // --- Text Inputs -------------------------------------------------------------

    /// Draws a text input with a label and optional autofocus.
    bool InputTextWithLabel(const char* label, char* buffer, size_t bufferSize, bool autoFocus = false);

    /// Draws a validated text input that highlights invalid content.
    bool InputTextWithLabelValidated(const char* label, char* buffer, size_t bufferSize, const std::function<bool(const char*)>& validator, bool autoFocus = false, bool* outIsValid = nullptr);

    // --- Lists -------------------------------------------------------------------

    /// Renders a scrollable list of selectable string entries.
    void DrawScrollableList(const std::vector<std::string>& items, float height, const std::string& selected, std::string& outSelection);

    /// Renders a scrollable list with a callback invoked for hovered items.
    void DrawScrollableList(const std::vector<std::string>& items, float height, const std::string& selected, std::string& outSelection, const ListItemCallback& onItemHover);

    // --- Trees -------------------------------------------------------------------

    /// Recursively draws a tree hierarchy and updates the selected node.
    void DrawTreeRecursive(const std::vector<TreeNodeData>& roots, std::string& selectedNode);

    /// Recursively draws a tree hierarchy while providing a context callback.
    void DrawTreeRecursive(const std::vector<TreeNodeData>& roots, std::string& selectedNode, const TreeNodeCallback& onContextMenu);

    /// Draws a script hierarchy tree or an informative message when empty.
    void DrawScriptHierarchyTree(const std::vector<TreeNodeData>& roots, std::string& selectedNode, const char* emptyMessage = "No entries");

    /// Draws a script hierarchy tree with context callbacks.
    void DrawScriptHierarchyTree(const std::vector<TreeNodeData>& roots, std::string& selectedNode, const TreeNodeCallback& onContextMenu, const char* emptyMessage = "No entries");

    // --- Style helpers -----------------------------------------------------------

    /// Pushes the smallest available font onto the font stack.
    void PushSmallFont();

    /// Pops the last font pushed with PushSmallFont.
    void PopSmallFont();

    /// Draws a separator with centred text.
    void DrawSeparatorText(const char* text);

    /// Draws a square icon-styled button with an optional tooltip.
    bool IconButton(const char* icon, const char* tooltip = nullptr);
}

