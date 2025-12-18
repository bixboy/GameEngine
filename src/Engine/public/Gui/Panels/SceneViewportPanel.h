#pragma once
#include "Gui/Base/GuiPanelBase.h"
#include "Gui/Core/DefaultEngineGui.h"
#include "Math/Vector2.h"
#include "imgui.h"

namespace BixEngine::Gui
{
    class SceneViewportPanel : public GuiPanelBase
    {
    public:
        explicit SceneViewportPanel(const DefaultEngineGuiContext& context);
        ~SceneViewportPanel() override = default;

        void Draw() override;

    private:
        DefaultEngineGuiContext context_;

        enum class GizmoMode { None, Translate, Rotate, Scale };
        enum class DragType { None, Center, XAxis, YAxis, Rotate, ScaleTopLeft, ScaleTopRight, ScaleBottomLeft, ScaleBottomRight };


        struct GizmoState
        {
            GizmoMode Mode{GizmoMode::Translate};
            DragType DraggingType{DragType::None};
            bool IsDragging{false};
            ImVec2 DragStartViewportPos{};
            float InitialDragAngle{0.0f};

            // Captured Transform
            struct InitialTransformData
            {
                float PosX, PosY;
                float Rot;
                float ScaleX, ScaleY;
            } InitialTransform{};
        } gizmoState_;

        void DrawGizmo(class Game::Actor* actor, const ImVec2& screenOffset, float viewScale);
        void HandleGizmoInteraction(class Game::Actor* actor, const ImVec2& screenOffset, const ImVec2& viewportMousePos, const ImVec2& viewScale);
        void HandleSelection(const ImVec2& viewportMousePos);

        // --- Helpers ---
        // Calculates the 4 World Space corners of the Actor's bounding box (OBB)
        void GetActorWorldCorners(class Game::Actor* actor, Math::Vector2<float>* outCorners) const;
        
        // Projects World Point to Screen Space
        ImVec2 WorldToScreen(const Math::Vector2<float>& worldPos, const ImVec2& screenOffset, float viewScale) const;
        
        // Rotates a 2D point (x,y) by radians
        static ImVec2 RotateVector(const ImVec2& vec, float radians);
        
        // Calculates distance from Point P to Line Segment AB
        static float DistanceToSegment(const ImVec2& P, const ImVec2& A, const ImVec2& B);

        // Individual Gizmo Drawers
        void DrawTranslateGizmo(ImDrawList* drawList, const ImVec2& screenCenter, float rotationRad) const;
        void DrawRotateGizmo(ImDrawList* drawList, const ImVec2& screenCenter, float rotationRad) const;
        void DrawScaleGizmo(ImDrawList* drawList, const ImVec2* screenCorners, float rotationRad) const;

        // Hit Testers
        DragType CheckGizmoHit(const ImVec2& mouseScreen, const ImVec2& screenCenter, const ImVec2* screenCorners, float rotationRad) const;
    };
}
