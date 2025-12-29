#include "Gui/Core/EditorPreferences.h"
#include "Gui/Core/GuiTheme.h"
#include "Utils/FileIO/BinaryUtils.h"
#include "imgui.h"
#include <fstream>
#include <filesystem>
#include "IO/FileUtils.h"

namespace BixEngine::Gui
{
    namespace
    {
        constexpr std::uint32_t kEditorSettingsVersion = 2;
        const char* kEditorSettingsFileName = "editor_settings.bin";
    }

    void SyncTheme(const EditorSettings& settings)
    {
        using namespace Theme;
        ContentBackground = settings.ThemeBackground;
        ContentTreeBackground = settings.ThemeBackground; // Linked to background for consistency
        HeaderBackground = settings.ThemeHeaderBackground;
        BreadcrumbHighlight = settings.ThemeAccentColor;
        SelectedFolderText = settings.ThemeAccentColor;

        AxisColorX = settings.AxisColorX;
        AxisColorY = settings.AxisColorY;
        AxisColorZ = settings.AxisColorZ;

        // Common panel backgrounds
        StatsBackground = settings.ThemeBackground;
        ViewportBackground = settings.ThemeBackground;
        OutlinerBackground = settings.ThemeBackground;
        InspectorBackground = settings.ThemeBackground;
        SectionBackground = settings.ThemeSectionBackground;
        OverviewBackground = settings.ThemeOverviewBackground;

        WarningColor = settings.ThemeWarningColor;
        ErrorColor = settings.ThemeErrorColor;
    }

    EditorSettings& EditorSettings::Get()
    {
        static EditorSettings instance;
        static bool bLoaded = false;
        if (!bLoaded)
        {
            bLoaded = true;
            instance.Load();
            SyncTheme(instance); // Ensure theme is synced after load
        }
        return instance;
    }

    std::filesystem::path EditorSettings::GetConfigPath()
    {
        std::filesystem::path path = std::filesystem::current_path() / "Config";
        if (!std::filesystem::exists(path))
            std::filesystem::create_directories(path);
        return path / kEditorSettingsFileName;
    }

    void EditorSettings::Save() const
    {
        std::ofstream file(GetConfigPath(), std::ios::binary);
        if (!file.is_open()) return;

        using namespace Utils;
        BinaryWriter writer(file);
        
        writer.WriteUint32(kEditorSettingsVersion);

        // Gizmos
        writer.WritePrimitive(EnableSnap);
        writer.WritePrimitive(GizmoSnapTranslation);
        writer.WritePrimitive(GizmoSnapRotation);
        writer.WritePrimitive(GizmoSnapScale);
        writer.WritePrimitive(GizmoHandleSize);
        writer.WritePrimitive(GizmoAxisLength);
        writer.WritePrimitive(GizmoRotateRadius);
        writer.WritePrimitive(GizmoLineThickness);
        writer.WritePrimitive(GizmoSensitivity);

        // Camera
        writer.WritePrimitive(CameraSpeed);
        writer.WritePrimitive(CameraBoostMultiplier);

        // Content Browser
        writer.WritePrimitive(ContentThumbnailSize);
        writer.WritePrimitive(ContentThumbnailPadding);
        writer.WritePrimitive(ContentTreeWidth);

        // Inspector
        writer.WritePrimitive(DragSpeedLocation);
        writer.WritePrimitive(DragSpeedRotation);
        writer.WritePrimitive(DragSpeedScale);

        // Stats
        writer.WritePrimitive(StatsSmoothingFactor);
        writer.WritePrimitive(StatsUpdateInterval);

        // Visuals
        writer.WritePrimitive(ShowGrid);
        writer.WritePrimitive(GridSpacing);
        writer.WritePrimitive(GridColor);
        writer.WritePrimitive(AxisColorX);
        writer.WritePrimitive(AxisColorY);
        writer.WritePrimitive(AxisColorZ);

        // Theme
        writer.WritePrimitive(ThemeBackground);
        writer.WritePrimitive(ThemeHeaderBackground);
        writer.WritePrimitive(ThemeSectionBackground);
        writer.WritePrimitive(ThemeOverviewBackground);
        writer.WritePrimitive(ThemeAccentColor);
        writer.WritePrimitive(ThemeWarningColor);
        writer.WritePrimitive(ThemeErrorColor);

        // Project
        // Convert to Bix::String for BinaryWriter
        writer.WriteString(String(DefaultMapPath.c_str())); 
    }

    void EditorSettings::Load()
    {
        std::ifstream file(GetConfigPath(), std::ios::binary);
        if (!file.is_open()) return;

        using namespace Utils;
        BinaryReader reader(file);

        std::uint32_t version = 0;
        if (!reader.ReadUint32(version)) return;

        // Gizmos
        reader.ReadPrimitive(EnableSnap);
        reader.ReadPrimitive(GizmoSnapTranslation);
        reader.ReadPrimitive(GizmoSnapRotation);
        reader.ReadPrimitive(GizmoSnapScale);
        reader.ReadPrimitive(GizmoHandleSize);
        reader.ReadPrimitive(GizmoAxisLength);
        reader.ReadPrimitive(GizmoRotateRadius);
        reader.ReadPrimitive(GizmoLineThickness);
        reader.ReadPrimitive(GizmoSensitivity);

        // Camera
        reader.ReadPrimitive(CameraSpeed);
        reader.ReadPrimitive(CameraBoostMultiplier);

        // Content Browser
        reader.ReadPrimitive(ContentThumbnailSize);
        reader.ReadPrimitive(ContentThumbnailPadding);
        reader.ReadPrimitive(ContentTreeWidth);

        // Inspector
        reader.ReadPrimitive(DragSpeedLocation);
        reader.ReadPrimitive(DragSpeedRotation);
        reader.ReadPrimitive(DragSpeedScale);

        // Stats
        reader.ReadPrimitive(StatsSmoothingFactor);
        reader.ReadPrimitive(StatsUpdateInterval);

        // Visuals
        reader.ReadPrimitive(ShowGrid);
        reader.ReadPrimitive(GridSpacing);
        reader.ReadPrimitive(GridColor);
        reader.ReadPrimitive(AxisColorX);
        reader.ReadPrimitive(AxisColorY);
        reader.ReadPrimitive(AxisColorZ);

        // Theme
        reader.ReadPrimitive(ThemeBackground);
        reader.ReadPrimitive(ThemeHeaderBackground);
        reader.ReadPrimitive(ThemeSectionBackground);
        reader.ReadPrimitive(ThemeOverviewBackground);
        reader.ReadPrimitive(ThemeAccentColor);
        reader.ReadPrimitive(ThemeWarningColor);
        reader.ReadPrimitive(ThemeErrorColor);

        // Project
        if (version >= 2)
        {
             String tempPath;
             if (reader.ReadString(tempPath))
             {
                 DefaultMapPath = tempPath.size() > 0 ? tempPath.c_str() : "";
             }
        }
    }

    void EditorPreferencesWindow::Draw(bool* open)
    {
        if (!*open) return;

        ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Editor Preferences", open))
        {
            auto& settings = EditorSettings::Get();

            if (ImGui::CollapsingHeader("Gizmos", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::Checkbox("Enable Snapping", &settings.EnableSnap)) settings.Save();
                ImGui::BeginDisabled(!settings.EnableSnap);
                if (ImGui::DragFloat("Snap Translation", &settings.GizmoSnapTranslation, 0.1f, 0.0f, 100.0f, "%.1f units")) settings.Save();
                if (ImGui::DragFloat("Snap Rotation", &settings.GizmoSnapRotation, 1.0f, 0.0f, 180.0f, "%.0f deg")) settings.Save();
                if (ImGui::DragFloat("Snap Scale", &settings.GizmoSnapScale, 0.01f, 0.0f, 10.0f, "%.2f")) settings.Save();
                ImGui::EndDisabled();

                ImGui::Separator();
                ImGui::Text("Appearance");
                if (ImGui::DragFloat("Handle Size", &settings.GizmoHandleSize, 0.5f, 1.0f, 50.0f)) settings.Save();
                if (ImGui::DragFloat("Axis Length", &settings.GizmoAxisLength, 1.0f, 10.0f, 500.0f)) settings.Save();
                if (ImGui::DragFloat("Rotation Radius", &settings.GizmoRotateRadius, 1.0f, 10.0f, 200.0f)) settings.Save();
                if (ImGui::DragFloat("Line Thickness", &settings.GizmoLineThickness, 0.1f, 1.0f, 10.0f)) settings.Save();
                if (ImGui::DragFloat("Interaction Sensitivity", &settings.GizmoSensitivity, 0.1f, 1.0f, 20.0f)) settings.Save();
            }

            if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::DragFloat("Camera Speed", &settings.CameraSpeed, 0.1f, 0.1f, 100.0f)) settings.Save();
                if (ImGui::DragFloat("Boost Multiplier", &settings.CameraBoostMultiplier, 0.1f, 1.0f, 10.0f)) settings.Save();
            }

            if (ImGui::CollapsingHeader("Content Browser", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::DragFloat("Thumbnail Size", &settings.ContentThumbnailSize, 1.0f, 32.0f, 256.0f)) settings.Save();
                if (ImGui::DragFloat("Thumbnail Padding", &settings.ContentThumbnailPadding, 1.0f, 0.0f, 64.0f)) settings.Save();
                if (ImGui::DragFloat("Tree Width", &settings.ContentTreeWidth, 1.0f, 100.0f, 500.0f)) settings.Save();
            }

            if (ImGui::CollapsingHeader("Inspector", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::DragFloat("Location Drag Speed", &settings.DragSpeedLocation, 0.01f, 0.001f, 10.0f)) settings.Save();
                if (ImGui::DragFloat("Rotation Drag Speed", &settings.DragSpeedRotation, 0.01f, 0.001f, 10.0f)) settings.Save();
                if (ImGui::DragFloat("Scale Drag Speed", &settings.DragSpeedScale, 0.01f, 0.001f, 10.0f)) settings.Save();
            }

            if (ImGui::CollapsingHeader("Performance Stats", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::SliderFloat("Smoothing Factor", &settings.StatsSmoothingFactor, 0.0f, 1.0f, "%.2f")) settings.Save();
                if (ImGui::DragFloat("Update Interval (s)", &settings.StatsUpdateInterval, 0.01f, 0.05f, 2.0f, "%.2fs")) settings.Save();
            }

            if (ImGui::CollapsingHeader("Visuals", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::SeparatorText("Viewport Grid");
                if (ImGui::Checkbox("Show Grid", &settings.ShowGrid)) settings.Save();
                if (ImGui::DragFloat("Grid Spacing", &settings.GridSpacing, 0.5f, 4.0f, 128.0f)) settings.Save();
                if (ImGui::ColorEdit4("Grid Color", (float*)&settings.GridColor, ImGuiColorEditFlags_NoInputs)) { SyncTheme(settings); settings.Save(); }
                
                ImGui::Spacing();
                ImGui::SeparatorText("Axis Colors");
                if (ImGui::ColorEdit4("Axis X", (float*)&settings.AxisColorX, ImGuiColorEditFlags_NoInputs)) { SyncTheme(settings); settings.Save(); }
                if (ImGui::ColorEdit4("Axis Y", (float*)&settings.AxisColorY, ImGuiColorEditFlags_NoInputs)) { SyncTheme(settings); settings.Save(); }
                if (ImGui::ColorEdit4("Axis Z", (float*)&settings.AxisColorZ, ImGuiColorEditFlags_NoInputs)) { SyncTheme(settings); settings.Save(); }
            }

            if (ImGui::CollapsingHeader("Theme", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::SeparatorText("Core Colors");
                if (ImGui::ColorEdit4("General Background", (float*)&settings.ThemeBackground, ImGuiColorEditFlags_NoInputs)) { SyncTheme(settings); settings.Save(); }
                if (ImGui::ColorEdit4("Header Background", (float*)&settings.ThemeHeaderBackground, ImGuiColorEditFlags_NoInputs)) { SyncTheme(settings); settings.Save(); }
                if (ImGui::ColorEdit4("Section Background", (float*)&settings.ThemeSectionBackground, ImGuiColorEditFlags_NoInputs)) { SyncTheme(settings); settings.Save(); }
                if (ImGui::ColorEdit4("Overview Panel", (float*)&settings.ThemeOverviewBackground, ImGuiColorEditFlags_NoInputs)) { SyncTheme(settings); settings.Save(); }
                
                ImGui::Spacing();
                ImGui::SeparatorText("Interactions & Feedback");
                if (ImGui::ColorEdit4("Accent Color", (float*)&settings.ThemeAccentColor, ImGuiColorEditFlags_NoInputs)) { SyncTheme(settings); settings.Save(); }
                if (ImGui::ColorEdit4("Warning Tint", (float*)&settings.ThemeWarningColor, ImGuiColorEditFlags_NoInputs)) { SyncTheme(settings); settings.Save(); }
                if (ImGui::ColorEdit4("Error Tint", (float*)&settings.ThemeErrorColor, ImGuiColorEditFlags_NoInputs)) { SyncTheme(settings); settings.Save(); }
            }

            if (ImGui::CollapsingHeader("Project", ImGuiTreeNodeFlags_DefaultOpen))
            {
                // InputText for std::string
                // Buffer resizing logic or simple static buffer?
                // imgui_stdlib.h usually provides support for std::string directly.
                // If not, we use a temp buffer. Assuming stdlib support or basic buffer.
                // Let's assume user has ImGui STL support or use a char buffer.
                // Checking previous includes... imgui.h is included.
                // If imgui_stdlib.h is missing, we must implement a wrapper.
                // BixEngine ImGui usually has helpers?
                // Safest is to use a fixed buffer and assign back.
                
                static char buffer[256];
                strncpy_s(buffer, settings.DefaultMapPath.c_str(), _TRUNCATE);
                if (ImGui::InputText("Default Map", buffer, sizeof(buffer)))
                {
                    settings.DefaultMapPath = buffer;
                    settings.Save();
                }
                ImGui::SameLine();
                if (ImGui::Button("Browse..."))
                {
                    // Filter: Display Name \0 Pattern \0 ... \0
                    const char* filter = "Scene Files (*.bix)\0*.bix\0All Files (*.*)\0*.*\0";
                    std::string path = BixEngine::Core::OpenFileDialog(filter); // Fixed namespace
                    if (!path.empty())
                    {
                        settings.DefaultMapPath = path;
                        settings.Save();
                        // Update buffer for immediate feedback if relying on static buffer
                        strncpy_s(buffer, path.c_str(), _TRUNCATE);
                    }
                }
                ImGui::SameLine();
                // Simple helper to verify if file exists
                if (!settings.DefaultMapPath.empty())
                {
                    if (std::filesystem::exists(settings.DefaultMapPath))
                        ImGui::TextColored({0,1,0,1}, "[Valid]");
                    else
                        ImGui::TextColored({1,0,0,1}, "[Invalid]");
                }
            }
        }
        ImGui::End();
    }
}



