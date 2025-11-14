#pragma once
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "Gui/GuiTheme.h"
#include "Gui/Widgets/Styling/ScopedColor.h"
#include "Gui/Widgets/Styling/ScopedStyle.h"
#include "imgui.h"
#include "Utils/ScriptUtils.h"


namespace BixEngine::Gui::Utils
{
    using ScriptUtils::TreeNodeData;

    using TreeNodeCallback = std::function<void(const TreeNodeData&)>;
    using ListItemCallback = std::function<void(const std::string&)>;

    struct GuiStateManager
    {
        std::unordered_map<std::string, bool> PersistentSections;
        std::vector<std::string> SectionStack;

        int SmallFontPushCount = 0;
        ImFont* CachedSmallestFont = nullptr;

        static GuiStateManager& Get()
        {
            static GuiStateManager Instance;
            return Instance;
        }

        void Reset() noexcept
        {
            PersistentSections.clear();
            SectionStack.clear();
            SmallFontPushCount = 0;
            CachedSmallestFont = nullptr;
        }
    };

    /// Gère automatiquement PushID / PopID.
    struct ScopedID
    {
        explicit ScopedID(int id) { ImGui::PushID(id); }
        explicit ScopedID(const void* id) { ImGui::PushID(id); }
        explicit ScopedID(const char* id) { ImGui::PushID(id); }
        ~ScopedID() { ImGui::PopID(); }
    };

    using ScopedStyle = Widgets::ScopedStyle;
    using ScopedColor = Widgets::ScopedColor;

    class ScopedStyleColor
    {
    public:
        ScopedStyleColor(ImGuiCol color, const ImVec4& value)
        {
            colorScope_.emplace(color, value);
        }

        ScopedStyleColor(ImGuiCol color, const ImVec4& value, bool condition)
        {
            if (condition)
            {
                colorScope_.emplace(color, value);
            }
        }

    private:
        std::optional<Widgets::ScopedColor> colorScope_{};
    };

    /// Gère automatiquement PushFont / PopFont.
    struct ScopedFont
    {
        explicit ScopedFont(ImFont* font)
        {
            if (font)
            {
                ImGui::PushFont(font);
                GuiStateManager::Get().SmallFontPushCount++;
            }
        }

        ~ScopedFont()
        {
            auto& state = GuiStateManager::Get();
            if (state.SmallFontPushCount > 0)
            {
                ImGui::PopFont();
                state.SmallFontPushCount--;
            }
        }
    };

    // ────────────────────────────────────────────────────────────────
    // affichage de sections et messages
    // ────────────────────────────────────────────────────────────────

    void DrawSectionHeader(const char* title);
    void DrawErrorMessage(const std::string& message);

    bool DrawConfirmButtons(const char* okLabel, const char* cancelLabel);
    bool DrawConfirmButtons(const char* okLabel, const char* cancelLabel, const std::function<void()>& onConfirm, const std::function<void()>& onCancel);

    // ────────────────────────────────────────────────────────────────
    // Champs texte, étiquettes et infos
    // ────────────────────────────────────────────────────────────────

    bool InputTextWithLabel(const char* label, char* buffer, size_t bufferSize, ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue, bool autoFocus = false);

    bool InputTextWithLabelValidated(const char* label, char* buffer, size_t bufferSize,const std::function<bool(const char*)>& validator,
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue, bool autoFocus = false, bool* outIsValid = nullptr);

    void DrawDescriptionText(const char* text);
    void DrawLabelValue(const char* label, const std::string& value, const char* emptyFallback = "-");
    void DrawEmptyStateMessage(const char* message);

    // ────────────────────────────────────────────────────────────────
    // Listes et arbres
    // ────────────────────────────────────────────────────────────────

    void DrawScrollableList(const std::vector<std::string>& items, float height, const std::string& selected, std::string& outSelection);
    void DrawScrollableList(const std::vector<std::string>& items, float height,const std::string& selected, std::string& outSelection,
        const ListItemCallback& onItemHover);

    void DrawTreeRecursive(const std::vector<TreeNodeData>& roots, std::string& selectedNode);
    void DrawTreeRecursive(const std::vector<TreeNodeData>& roots, std::string& selectedNode, const TreeNodeCallback& onContextMenu);

    void DrawScriptHierarchyTree(const std::vector<TreeNodeData>& roots, std::string& selectedNode, const char* emptyMessage = "No entries");
    void DrawScriptHierarchyTree(const std::vector<TreeNodeData>& roots, std::string& selectedNode,
        const TreeNodeCallback& onContextMenu, const char* emptyMessage = "No entries");

    // ────────────────────────────────────────────────────────────────
    // Sections repliables
    // ────────────────────────────────────────────────────────────────

    bool BeginCollapsibleSection(const char* label, bool defaultOpen = true, ImGuiTreeNodeFlags additionalFlags = 0);
    bool BeginPersistentSection(const char* label, const std::string& contextId, bool defaultOpen = true, ImGuiTreeNodeFlags additionalFlags = 0);
    void EndPersistentSection();

    // ────────────────────────────────────────────────────────────────
    // Recherche et mini-widgets
    // ────────────────────────────────────────────────────────────────

    bool SearchInput(const char* id, char* buffer, size_t bufferSize, const char* hint = "Search...",
                     float width = -1.0f, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None);

    bool IconButton(const char* icon, const char* tooltip = nullptr);
    void DrawHelpMarker(const char* text);

    // ────────────────────────────────────────────────────────────────
    // Gestion des polices et séparateurs
    // ────────────────────────────────────────────────────────────────

    void PushSmallFont();
    void PopSmallFont();
    void DrawSeparatorText(const char* text);

    ImVec4 AdjustColor(const ImVec4& color, float delta) noexcept;

    void ShowTooltip(const char* text, ImGuiHoveredFlags flags = Theme::TooltipHoverFlags);
    bool IsItemDoubleClicked(ImGuiMouseButton button = ImGuiMouseButton_Left, ImGuiHoveredFlags flags = Theme::DoubleClickHoverFlags) noexcept;

    ImTextureRef ToTextureRef(void* nativeHandle);
}
