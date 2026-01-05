#pragma once
#include "GuiPanelBase.h"
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

        void DrawBody() override;

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

            struct InitialTransformData
            {
                float PosX, PosY;
                float Rot;
                float ScaleX, ScaleY;
            }
            InitialTransform{};
        }
        gizmoState_;

        void DrawGizmo(Game::Actor* actor, const ImVec2& screenOffset, float viewScale);
        void HandleGizmoInteraction(Game::Actor* actor, const ImVec2& screenOffset, const ImVec2& viewportMousePos, const ImVec2& viewScale);
        
        void HandleSelection(const ImVec2& viewportMousePos);
        void GetActorWorldCorners(Game::Actor* actor, Math::Vector2<float>* outCorners) const;
        
        ImVec2 WorldToScreen(const Math::Vector2<float>& worldPos, const ImVec2& screenOffset, float viewScale) const;
        static ImVec2 RotateVector(const ImVec2& vec, float radians);
        static float DistanceToSegment(const ImVec2& P, const ImVec2& A, const ImVec2& B);

        void DrawTranslateGizmo(ImDrawList* drawList, const ImVec2& screenCenter, float rotationRad) const;
        void DrawRotateGizmo(ImDrawList* drawList, const ImVec2& screenCenter, float rotationRad) const;
        void DrawScaleGizmo(ImDrawList* drawList, const ImVec2* screenCorners, float rotationRad) const;
        
        DragType CheckGizmoHit(const ImVec2& mouseScreen, const ImVec2& screenCenter, const ImVec2* screenCorners, float rotationRad) const;
    };
}