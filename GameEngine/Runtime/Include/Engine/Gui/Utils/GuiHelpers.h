#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "Engine/Gui/GuiTheme.h"
#include "imgui.h"

namespace BixEngine::Gui::Utils
{
    // ────────────────────────────────────────────────────────────────
    // 🧩 Structures de base : nœuds et callbacks
    // ────────────────────────────────────────────────────────────────

    struct TreeNodeData
    {
        std::string name;                      // Nom affiché du nœud
        std::vector<TreeNodeData> children;    // Liste des enfants (sous-nœuds)
        bool isLeaf = false;                   // Indique si c’est une feuille terminale
    };

    /// Callback déclenché lors d’une action (clic droit, menu...) sur un nœud.
    using TreeNodeCallback = std::function<void(const TreeNodeData&)>;

    /// Callback déclenché lors du survol d’un élément de liste.
    using ListItemCallback = std::function<void(const std::string&)>;

    // ────────────────────────────────────────────────────────────────
    // 🧱 Gestionnaire d’état global ImGui
    // ────────────────────────────────────────────────────────────────

    struct GuiStateManager
    {
        std::unordered_map<std::string, bool> PersistentSections;  // État ouvert/fermé des sections
        std::vector<std::string> SectionStack;                     // Pile des sections imbriquées
        int SmallFontPushCount = 0;                                // Nombre de polices poussées
        ImFont* CachedSmallestFont = nullptr;                      // Police la plus petite en cache

        /// Accès global (singleton interne)
        static GuiStateManager& Get()
        {
            static GuiStateManager Instance;
            return Instance;
        }

        /// Réinitialise tout l’état interne
        void Reset() noexcept
        {
            PersistentSections.clear();
            SectionStack.clear();
            SmallFontPushCount = 0;
            CachedSmallestFont = nullptr;
        }
    };

    // ────────────────────────────────────────────────────────────────
    // ⚙️ RAII Helpers : push/pop automatiques
    // ────────────────────────────────────────────────────────────────

    /// Gère automatiquement PushID / PopID.
    struct ScopedID
    {
        explicit ScopedID(int id) { ImGui::PushID(id); }
        explicit ScopedID(const void* id) { ImGui::PushID(id); }
        explicit ScopedID(const char* id) { ImGui::PushID(id); }
        ~ScopedID() { ImGui::PopID(); }
    };

    /// Gère automatiquement PushStyleVar / PopStyleVar.
    struct ScopedStyle
    {
        ScopedStyle(ImGuiStyleVar var, const ImVec2& value) { ImGui::PushStyleVar(var, value); }
        ScopedStyle(ImGuiStyleVar var, float value) { ImGui::PushStyleVar(var, value); }
        ~ScopedStyle() { ImGui::PopStyleVar(); }
    };

    /// Gère automatiquement PushStyleColor / PopStyleColor.
    struct ScopedColor
    {
        ScopedColor(ImGuiCol color, const ImVec4& value) { ImGui::PushStyleColor(color, value); }
        ~ScopedColor() { ImGui::PopStyleColor(); }
    };

    struct ScopedStyleColor
    {
        ScopedStyleColor(ImGuiCol color, const ImVec4& value) : applied(true)
        {
            ImGui::PushStyleColor(color, value);
        }

        ScopedStyleColor(ImGuiCol color, const ImVec4& value, bool condition) : applied(condition)
        {
            if (applied)
            {
                ImGui::PushStyleColor(color, value);
            }
        }

        ~ScopedStyleColor()
        {
            if (applied)
            {
                ImGui::PopStyleColor();
            }
        }

    private:
        bool applied;
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
    // 🎨 Fonctions de base : affichage de sections et messages
    // ────────────────────────────────────────────────────────────────

    void DrawSectionHeader(const char* title);         // Titre avec séparateurs haut/bas
    void DrawErrorMessage(const std::string& message); // Message d’erreur rouge

    bool DrawConfirmButtons(const char* okLabel, const char* cancelLabel); // Boutons OK / Cancel
    bool DrawConfirmButtons(const char* okLabel, const char* cancelLabel,
                            const std::function<void()>& onConfirm,
                            const std::function<void()>& onCancel);

    // ────────────────────────────────────────────────────────────────
    // ✏️ Champs texte, étiquettes et infos
    // ────────────────────────────────────────────────────────────────

    bool InputTextWithLabel(const char* label, char* buffer, size_t bufferSize, ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue, bool autoFocus = false);

    bool InputTextWithLabelValidated(const char* label, char* buffer, size_t bufferSize,
                                     const std::function<bool(const char*)>& validator,
                                     ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue,
                                     bool autoFocus = false, bool* outIsValid = nullptr);

    void DrawDescriptionText(const char* text); // Texte descriptif (wrap)
    void DrawLabelValue(const char* label, const std::string& value, const char* emptyFallback = "-"); // Label + valeur
    void DrawEmptyStateMessage(const char* message); // Message grisé "Empty"

    // ────────────────────────────────────────────────────────────────
    // 📜 Listes et arbres (rendu et interaction)
    // ────────────────────────────────────────────────────────────────

    void DrawScrollableList(const std::vector<std::string>& items, float height,
                            const std::string& selected, std::string& outSelection);
    void DrawScrollableList(const std::vector<std::string>& items, float height,
                            const std::string& selected, std::string& outSelection, const ListItemCallback& onItemHover);

    void DrawTreeRecursive(const std::vector<TreeNodeData>& roots, std::string& selectedNode);
    void DrawTreeRecursive(const std::vector<TreeNodeData>& roots, std::string& selectedNode, const TreeNodeCallback& onContextMenu);

    void DrawScriptHierarchyTree(const std::vector<TreeNodeData>& roots, std::string& selectedNode,
                                 const char* emptyMessage = "No entries");
    void DrawScriptHierarchyTree(const std::vector<TreeNodeData>& roots, std::string& selectedNode,
                                 const TreeNodeCallback& onContextMenu, const char* emptyMessage = "No entries");

    // ────────────────────────────────────────────────────────────────
    // 🔽 Sections repliables (persistantes ou non)
    // ────────────────────────────────────────────────────────────────

    bool BeginCollapsibleSection(const char* label, bool defaultOpen = true, ImGuiTreeNodeFlags additionalFlags = 0); // Section simple
    bool BeginPersistentSection(const char* label, const std::string& contextId, bool defaultOpen = true, ImGuiTreeNodeFlags additionalFlags = 0); // État sauvegardé
    void EndPersistentSection(); // Ferme la section

    // ────────────────────────────────────────────────────────────────
    // 🔍 Recherche et mini-widgets
    // ────────────────────────────────────────────────────────────────

    bool SearchInput(const char* id, char* buffer, size_t bufferSize, const char* hint = "Search...",
                     float width = -1.0f, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None); // Champ recherche

    bool IconButton(const char* icon, const char* tooltip = nullptr); // Bouton carré avec tooltip
    void DrawHelpMarker(const char* text);                            // "?" contextuel (tooltip)

    // ────────────────────────────────────────────────────────────────
    // 🔤 Gestion des polices et séparateurs
    // ────────────────────────────────────────────────────────────────

    void PushSmallFont();                // Active la plus petite police dispo
    void PopSmallFont();                 // Restaure la police précédente
    void DrawSeparatorText(const char* text); // Séparateur textuel centré

    ImVec4 AdjustColor(const ImVec4& color, float delta) noexcept;

    void ShowTooltip(const char* text, ImGuiHoveredFlags flags = Theme::TooltipHoverFlags);
    bool IsItemDoubleClicked(ImGuiMouseButton button = ImGuiMouseButton_Left,
                             ImGuiHoveredFlags flags = Theme::DoubleClickHoverFlags) noexcept;

}
