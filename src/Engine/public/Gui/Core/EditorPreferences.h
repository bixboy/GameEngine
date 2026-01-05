#pragma once
#include <SDL3/SDL.h>
#include <filesystem>
#include <string>
#include "imgui.h"


namespace BixEngine::Gui
{
    struct EditorSettings
    {
        bool EnableSnap = false;
        float GizmoSnapTranslation = 0.5f;
        float GizmoSnapRotation = 15.0f; 
        float GizmoSnapScale = 0.1f;
        
        float GizmoHandleSize = 10.0f;
        float GizmoAxisLength = 100.0f;
        float GizmoRotateRadius = 50.0f;
        float GizmoLineThickness = 2.0f;
        float GizmoSensitivity = 8.0f;
        
        float ContentThumbnailSize = 72.0f;
        float ContentThumbnailPadding = 28.0f;
        float ContentTreeWidth = 240.0f;
        
        float DragSpeedLocation = 0.1f;
        float DragSpeedRotation = 0.1f;
        float DragSpeedScale = 0.05f;
        
        float StatsSmoothingFactor = 0.1f;
        float StatsUpdateInterval = 0.25f;
        
        float CameraSpeed = 10.0f;
        float CameraBoostMultiplier = 2.0f;
        
        bool ShowGrid = true;
        float GridSpacing = 24.0f;
        ImVec4 GridColor = ImVec4{0.2f, 0.2f, 0.2f, 1.0f};
        ImVec4 AxisColorX = ImVec4{0.8f, 0.28f, 0.28f, 1.0f};
        ImVec4 AxisColorY = ImVec4{0.32f, 0.72f, 0.45f, 1.0f};
        ImVec4 AxisColorZ = ImVec4{0.26f, 0.45f, 0.86f, 1.0f};
        
        ImVec4 ThemeBackground = ImVec4{0.09f, 0.09f, 0.09f, 0.95f};
        ImVec4 ThemeHeaderBackground = ImVec4{0.16f, 0.16f, 0.16f, 1.0f};
        ImVec4 ThemeSectionBackground = ImVec4{0.12f, 0.12f, 0.12f, 1.0f};
        ImVec4 ThemeOverviewBackground = ImVec4{0.14f, 0.14f, 0.14f, 1.0f};
        ImVec4 ThemeAccentColor = ImVec4{0.95f, 0.80f, 0.40f, 1.0f}; 
        ImVec4 ThemeWarningColor = ImVec4{0.90f, 0.75f, 0.20f, 1.0f};
        ImVec4 ThemeErrorColor = ImVec4{0.90f, 0.30f, 0.30f, 1.0f};
        
        std::string DefaultMapPath = ""; 
        
        void Save() const;
        void Load();
        
        static std::filesystem::path GetConfigPath();

        static EditorSettings& Get();
    };
     
    class EditorPreferencesWindow
    {
    public:
        static void Draw(bool* open);
    };
}
