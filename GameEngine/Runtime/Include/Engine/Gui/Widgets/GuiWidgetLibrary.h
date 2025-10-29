#pragma once

#include <functional>
#include <string>
#include <vector>

#include "imgui.h"

namespace BixEngine::Gui::Widgets
{
    // ────────────────────────────────────────────────────────────────
    // 🎨 Structures de données de base pour les widgets
    // ────────────────────────────────────────────────────────────────

    /**
     * Représente une métrique ou donnée de performance à afficher sous forme de ligne.
     * Exemple : FPS, Frame Time, Draw Calls...
     */
    struct MetricDisplay
    {
        std::string label{};   // Nom de la métrique
        std::string value{};   // Valeur formatée (ex: "60 FPS" ou "16.7 ms")
        ImVec4 color{1.0f, 1.0f, 1.0f, 1.0f}; // Couleur du texte
        std::string hint{};    // Info-bulle optionnelle
    };

    /**
     * Options d’affichage pour un en-tête de panneau.
     */
    struct PanelHeaderOptions
    {
        std::string title{};       // Titre principal
        std::string subtitle{};    // Sous-titre
        bool showSeparator{true};  // Afficher une ligne de séparation
    };

    // ────────────────────────────────────────────────────────────────
    // 🧩 Fonctions utilitaires pour dessiner des widgets simples
    // ────────────────────────────────────────────────────────────────

    /// Dessine un en-tête de panneau (titre + sous-titre + séparateur optionnel)
    void DrawPanelHeader(const PanelHeaderOptions& options);

    /// Dessine une ligne de tableau contenant une métrique.
    void DrawMetricRow(const MetricDisplay& metric);

    /// Dessine un tableau complet de métriques (label + valeur + couleur)
    void DrawMetricsTable(const std::vector<MetricDisplay>& metrics,
                          float columnWidth = 140.0f,
                          const char* id = "MetricsTable");

    // ────────────────────────────────────────────────────────────────
    // 🧱 RAII : Section repliable / collapsible
    // ────────────────────────────────────────────────────────────────

    /**
     * Encapsule un header ImGui::CollapsingHeader() avec gestion automatique de la fermeture.
     */
    class PanelSection
    {
    public:
        explicit PanelSection(const char* label, bool defaultOpen = true, ImGuiTreeNodeFlags flags = 0);
        ~PanelSection();

        PanelSection(const PanelSection&) = delete;
        PanelSection& operator=(const PanelSection&) = delete;
        PanelSection(PanelSection&&) noexcept = delete;
        PanelSection& operator=(PanelSection&&) noexcept = delete;

        [[nodiscard]] bool IsOpen() const noexcept { return open_; }

    private:
        bool open_{false};
    };

    // ────────────────────────────────────────────────────────────────
    // 🧰 RAII : Barre d’outils horizontale pour panneaux
    // ────────────────────────────────────────────────────────────────

    /**
     * Barre d’outils horizontale divisée en deux zones (gauche / droite).
     * Chaque côté accepte des callbacks de dessin ImGui.
     * Appel à Commit() pour finaliser et tracer le séparateur.
     */
    class PanelToolbar
    {
    public:
        PanelToolbar() = default;
        ~PanelToolbar() = default;

        PanelToolbar(const PanelToolbar&) = delete;
        PanelToolbar& operator=(const PanelToolbar&) = delete;
        PanelToolbar(PanelToolbar&&) noexcept = delete;
        PanelToolbar& operator=(PanelToolbar&&) noexcept = delete;

        /// Ajoute un élément à gauche de la barre (icône, texte, bouton…)
        void AddLeft(const std::function<void()>& drawCallback);

        /// Ajoute un élément à droite de la barre
        void AddRight(const std::function<void()>& drawCallback);

        /// Finalise la barre
        void Commit();

    private:
        std::vector<std::function<void()>> leftElements_;
        std::vector<std::function<void()>> rightElements_;
        bool committed_{false};
    };

    // ────────────────────────────────────────────────────────────────
    // 🎨 RAII de style — gestion automatique des Push/Pop
    // ────────────────────────────────────────────────────────────────

    /**
     * Classe utilitaire permettant de gérer automatiquement un PushStyleVar/PopStyleVar.
     */
    class ScopedStyle
    {
    public:
        ScopedStyle(ImGuiStyleVar var, const ImVec2& value) { ImGui::PushStyleVar(var, value); }
        ScopedStyle(ImGuiStyleVar var, float value) { ImGui::PushStyleVar(var, value); }
        ~ScopedStyle() { ImGui::PopStyleVar(); }
    };

    /**
     * Classe utilitaire RAII pour PushStyleColor/PopStyleColor.
     */
    class ScopedColor
    {
    public:
        ScopedColor(ImGuiCol colorIndex, const ImVec4& color) { ImGui::PushStyleColor(colorIndex, color); }
        ~ScopedColor() { ImGui::PopStyleColor(); }
    };

    // ────────────────────────────────────────────────────────────────
    // 🌈 Couleurs standard pour indicateurs de performance
    // ────────────────────────────────────────────────────────────────

    /// Renvoie une couleur verte (bon état)
    inline ImVec4 GoodColor() noexcept { return {0.2f, 1.0f, 0.2f, 1.0f}; }

    /// Renvoie une couleur jaune (avertissement)
    inline ImVec4 WarnColor() noexcept { return {1.0f, 1.0f, 0.2f, 1.0f}; }

    /// Renvoie une couleur rouge (mauvais état)
    inline ImVec4 BadColor() noexcept { return {1.0f, 0.2f, 0.2f, 1.0f}; }

    // ────────────────────────────────────────────────────────────────
    // 🧱 Namespace Builder (API fluide / DSL interne)
    // ────────────────────────────────────────────────────────────────

    /**
     * Namespace inline pour des builders RAII.
     * Permet d’écrire : Widgets::Section("Transform");
     */
    inline namespace Builder
    {
        /**
         * Builder RAII pour créer une section collapsible avec une syntaxe fluide.
         */
        class Section
        {
        public:
            Section(const char* label, bool defaultOpen = true, ImGuiTreeNodeFlags flags = 0);
            ~Section();

            Section(const Section&) = delete;
            Section& operator=(const Section&) = delete;
            Section(Section&&) noexcept = delete;
            Section& operator=(Section&&) noexcept = delete;

            [[nodiscard]] bool IsOpen() const noexcept { return section_.IsOpen(); }

        private:
            PanelSection section_;
        };
    }

    // ────────────────────────────────────────────────────────────────
    //  - DrawPropertyGrid()
    //  - DrawVector3Editor()
    //  - DrawTexturePreview()
    //  - DrawCollapsibleCard()
    // ────────────────────────────────────────────────────────────────
}
