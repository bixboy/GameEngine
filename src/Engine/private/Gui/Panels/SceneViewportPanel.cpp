#include "Gui/Panels/SceneViewportPanel.h"
#include "Gui/Core/EditorPreferences.h"
#include <SDL3/SDL_render.h>
#include <numbers>
#include <utility>
#include <cmath> 

#include "imgui.h"
#include "Gui/Utils/GuiHelpers.h"
#include "Framework/Actor.h"
#include "Components/Sprite/SpriteComponent.h"
#include "Components/Core/BoxColliderComponent.h"
#include "Components/Core/CameraComponent.h"
#include "Serializer/SceneSerializer.h"
#include "Utils/FileIO/BinaryUtils.h"
#include "Serializer/PrefabSerializer.h"
#include "Debug/Logger.h"
#include <filesystem>
#include <fstream>
#include "Math/Matrix/Matrix3.h"


namespace BixEngine::Gui
{
    using namespace Utils;

    namespace
    {
        std::unique_ptr<Game::Actor> LoadActorBinary(const std::filesystem::path& path)
        {
            std::ifstream file(path, std::ios::binary);
            if (file.is_open())
            {
                BinaryReader reader(file);
                
                String typeName;
                std::unique_ptr<Game::Actor> actor = nullptr;

                if (reader.ReadString(typeName) && !typeName.empty())
                {
                    actor = Serialization::SceneSerializer::CreateActor(typeName);
                    if (!actor)
                    {
                        LOG_WARNING("LoadActorBinary: Could not create actor of type " + typeName + ". Fallback to Game::Actor.");
                        actor = std::make_unique<Game::Actor>("Root");
                    }
                }
                else
                {
                    file.clear();
                    file.seekg(0, std::ios::beg); 
                    actor = std::make_unique<Game::Actor>("Root");
                }

                try
                {
                    actor->DeserializeBinary(file);
                    return actor;
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR("[SceneViewportPanel] Failed to load prefab binary: " + path.generic_string() + ". Error: " + e.what());
                }
            }
            return nullptr;
        }

        ImVec2 ComputeImageSize(const ImVec2& availableSize, int textureWidth, int textureHeight)
        {
            if (availableSize.x <= 0.0f || availableSize.y <= 0.0f || textureWidth <= 0 || textureHeight <= 0)
                return ImVec2{0,0};

            const float texAspect = static_cast<float>(textureWidth) / textureHeight;
            const float availAspect = availableSize.x / availableSize.y;

            ImVec2 final = availableSize;
            if (availAspect > texAspect)
            {
                final.x = final.y * texAspect;
            }
            else
            {
                final.y = final.x / texAspect;
            }

            return final;
        }

        void DrawTexture(SDL_Texture* texture, const ImVec2& pos, const ImVec2& size)
        {
            if (!texture || size.x <= 0 || size.y <= 0)
                return;

            ImGui::GetWindowDrawList()->AddImage(
                texture,
                pos,
                ImVec2(pos.x + size.x, pos.y + size.y)
            );
        }
    }
    
    SceneViewportPanel::SceneViewportPanel(const DefaultEngineGuiContext& context) : GuiPanelBase("scene_viewport"), context_(context)
    {
    }
    
    void SceneViewportPanel::DrawBody()
    {
        GuiUtils::ScopedID id("SceneViewport");

        const ImVec2 avail = ImGui::GetContentRegionAvail();
        const ImVec2 cursor = ImGui::GetCursorScreenPos();

        if (avail.x <= 0 || avail.y <= 0)
        {
            ImGui::TextUnformatted("Viewport unavailable.");
            return;
        }

        SDL_Texture* texture = context_.sceneRenderTextureProvider ? context_.sceneRenderTextureProvider() : nullptr;
        const auto size = context_.sceneRenderTextureSizeProvider ? context_.sceneRenderTextureSizeProvider() : std::pair{0,0};

        if (!texture || size.first <= 0 || size.second <= 0)
        {
            ImGui::Dummy(avail);
            
            const ImVec2 txtSize = ImGui::CalcTextSize("No scene is currently available.");
            ImVec2 center = cursor;
            center.x += (avail.x - txtSize.x) * 0.5f;
            center.y += (avail.y - txtSize.y) * 0.5f;
            ImGui::GetWindowDrawList()->AddText(center, ImGui::GetColorU32(ImGuiCol_TextDisabled), "No scene is currently available.");
            
            return;
        }

        const ImVec2 imgSize = ComputeImageSize(avail, size.first, size.second);
        ImVec2 drawPos = cursor;
        drawPos.x += (avail.x - imgSize.x) * 0.5f;
        drawPos.y += (avail.y - imgSize.y) * 0.5f;

        DrawTexture(texture, drawPos, imgSize);
        
        ImGui::SetCursorScreenPos(cursor);
        ImGui::Dummy(avail);

        ImVec2 overlayPos = { drawPos.x + 12.f, drawPos.y + 12.f };
        ImGui::GetWindowDrawList()->AddText(overlayPos, ImGui::GetColorU32(ImVec4(1,1,1,0.8f)), "Scene Viewport");

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
            {
                std::filesystem::path path(static_cast<const char*>(payload->Data));
                if (path.extension() == ".bixactor")
                {
                    auto root = Serialization::PrefabSerializer::LoadPrefab(path);
                    if (root)
                    {
                        Game::Scene* scene = GetScene();
                        if (scene)
                        {
                            struct HierarchyNode
                            {
                                Game::Actor* actor;
                                Game::Actor* parent;
                            };
                            
                            std::vector<HierarchyNode> hierarchy;
                            std::vector<Game::Actor*> descendants;

                            std::function<void(Game::Actor*)> collect = [&](Game::Actor* node)
                            {
                                for(auto* child : node->GetChildren())
                                {
                                    hierarchy.push_back({child, node});
                                    descendants.push_back(child);
                                    collect(child);
                                }
                            };
                            collect(root.get());

                            Game::Actor* rootPtr = root.get();
                            scene->AddActor(std::move(root));

                            for(auto* child : descendants)
                            {
                                std::unique_ptr<Game::Actor> uChild(child);
                                scene->AddActor(std::move(uChild));
                            }

                            for(const auto& node : hierarchy)
                            {
                                if (node.actor && node.parent)
                                    node.actor->SetParent(node.parent);
                            }

                             if (context_.selectedActorSetter)
                                context_.selectedActorSetter(rootPtr);

                            LOG_INFO("Added Prefab to Scene: " + path.generic_string());
                        }
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }
        
        float scaleX = imgSize.x / static_cast<float>(size.first);
        float scaleY = imgSize.y / static_cast<float>(size.second);
        
        Game::Actor* selectedActor = context_.selectedActorGetter ? context_.selectedActorGetter() : nullptr;

        if (selectedActor)
        {
            DrawGizmo(selectedActor, drawPos, scaleX);
            
            ImVec2 mousePos = ImGui::GetMousePos();
            
            HandleGizmoInteraction(selectedActor, drawPos, mousePos, {scaleX, scaleY});
        }

        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0))
        {
             ImVec2 mousePos = ImGui::GetMousePos();
             
             ImVec2 localMouse = {
                 (mousePos.x - drawPos.x) / scaleX,
                 (mousePos.y - drawPos.y) / scaleY
             };
             
            if (localMouse.x >= 0 && localMouse.x <= size.first && localMouse.y >= 0 && localMouse.y <= size.second)
            {
                 if (!gizmoState_.IsDragging)
                 {
                     HandleSelection(localMouse);
                     selectedActor = context_.selectedActorGetter ? context_.selectedActorGetter() : nullptr; 
                 }
            }
        }

        ImGui::SetCursorScreenPos({cursor.x + 10, cursor.y + 40});
        
        auto ToggleButton = [&](const char* label, GizmoMode mode)
        {
            bool active = gizmoState_.Mode == mode;
            if (active)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.9f, 1.0f));
            
            if (ImGui::Button(label))
                gizmoState_.Mode = mode;
            
            if (active)
                ImGui::PopStyleColor();
        };

        ToggleButton("Translate", GizmoMode::Translate);
        
        ImGui::SameLine();
        
        ToggleButton("Rotate", GizmoMode::Rotate);
        
        ImGui::SameLine();
        
        ToggleButton("Scale", GizmoMode::Scale);
    }
    
    void SceneViewportPanel::HandleSelection(const ImVec2& viewportMousePos)
    {
         Game::Scene* scene = GetScene();
         if (!scene)
             return;
         
         const auto& actors = scene->GetActors();
         Game::CameraComponent* cam = Game::CameraComponent::GetMainCamera();

         for (auto it = actors.rbegin(); it != actors.rend(); ++it)
         {
             Game::Actor* actor = it->get();
             if (!actor)
                 continue;

             Math::Vector2 worldCorners[4];
             GetActorWorldCorners(actor, worldCorners);
             
             if (cam)
             {
                 for(int i=0; i<4; ++i)
                 {
                      Math::Vector3 p3(worldCorners[i].x, worldCorners[i].y, 0.0f);
                      Math::Vector2 screenPos = cam->WorldToScreen(p3);
                      worldCorners[i] = screenPos;
                 }
             }

             float wMinX = worldCorners[0].x, wMaxX = worldCorners[0].x;
             float wMinY = worldCorners[0].y, wMaxY = worldCorners[0].y;

             for(int i=1; i<4; ++i)
             {
                 if(worldCorners[i].x < wMinX)
                     wMinX = worldCorners[i].x;
                 
                 if(worldCorners[i].x > wMaxX)
                     wMaxX = worldCorners[i].x;
                 
                 if(worldCorners[i].y < wMinY)
                     wMinY = worldCorners[i].y;
                 
                 if(worldCorners[i].y > wMaxY)
                     wMaxY = worldCorners[i].y;
             }
             
             if (viewportMousePos.x >= wMinX && viewportMousePos.x <= wMaxX &&
                 viewportMousePos.y >= wMinY && viewportMousePos.y <= wMaxY)
             {
                 if (context_.selectedActorSetter)
                    context_.selectedActorSetter(actor);
                 
                 return;
             }
         }

         if (context_.selectedActorSetter)
             context_.selectedActorSetter(nullptr);
    }

    Game::Scene* SceneViewportPanel::GetScene() const
    {
        if (context_.sceneProvider)
            return context_.sceneProvider();
        
        if (context_.sceneManagerProvider)
        {
            if (auto* sm = context_.sceneManagerProvider())
                return sm->GetActiveScene();
        }
                
        return nullptr;
    }

    void SceneViewportPanel::DrawGizmo(Game::Actor* actor, const ImVec2& screenOffset, float viewScale)
    {
        Math::Vector2 worldCorners[4];
        GetActorWorldCorners(actor, worldCorners);

        ImVec2 screenCorners[4];
        for(int i=0; i<4; ++i)
        {
            screenCorners[i] = WorldToScreen(worldCorners[i], screenOffset, viewScale);
        }

        auto pos = actor->GetTransform().GetWorldPosition(); 
        ImVec2 screenCenter = WorldToScreen({pos.x, pos.y}, screenOffset, viewScale);
        
        float rotationRad = actor->GetTransform().GetWorldRotation().yaw * (std::numbers::pi_v<float> / 180.0f);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImU32 accentColor = ImGui::GetColorU32(EditorSettings::Get().ThemeAccentColor);
        drawList->AddQuad(screenCorners[0], screenCorners[1], screenCorners[2], screenCorners[3], accentColor, 2.0f);

        if (gizmoState_.Mode == GizmoMode::Scale)
        {
            DrawScaleGizmo(drawList, screenCorners, rotationRad);
        }
        else if (gizmoState_.Mode == GizmoMode::Rotate)
        {
            DrawRotateGizmo(drawList, screenCenter, rotationRad);
        }
        else if (gizmoState_.Mode == GizmoMode::Translate)
        {
            DrawTranslateGizmo(drawList, screenCenter, rotationRad);
        }
    }

    void SceneViewportPanel::HandleGizmoInteraction(Game::Actor* actor, const ImVec2& screenOffset, const ImVec2& viewportMousePos, const ImVec2& viewScale)
    {
        ImVec2 screenMouse = ImGui::GetMousePos();
        auto pos = actor->GetTransform().GetWorldPosition();
        float rotationRad = actor->GetTransform().GetWorldRotation().yaw * (std::numbers::pi_v<float> / 180.0f);
        
        ImVec2 screenCenter = WorldToScreen({pos.x, pos.y}, screenOffset, viewScale.x); 
        
        Math::Vector2 worldCorners[4];
        GetActorWorldCorners(actor, worldCorners);
        ImVec2 screenCorners[4];
        for(int i=0; i<4; ++i) screenCorners[i] = WorldToScreen(worldCorners[i], screenOffset, viewScale.x);

        if (ImGui::IsMouseDown(0))
        {
            if (!gizmoState_.IsDragging && !ImGui::IsMouseDragging(0, 0.5f))
            {
                 if (ImGui::IsMouseClicked(0))
                 {
                     DragType hitType = CheckGizmoHit(screenMouse, screenCenter, screenCorners, rotationRad);
                     if (hitType != DragType::None)
                     {
                         gizmoState_.IsDragging = true;
                         gizmoState_.DraggingType = hitType;
                         gizmoState_.DragStartViewportPos = viewportMousePos;
                         
                         Math::Transform t = actor->GetTransform();
                         gizmoState_.InitialTransform.PosX = t.GetWorldPosition().x;
                         gizmoState_.InitialTransform.PosY = t.GetWorldPosition().y;
                         gizmoState_.InitialTransform.Rot = t.GetWorldRotation().yaw;
                         gizmoState_.InitialTransform.ScaleX = t.GetWorldScale().x;
                         gizmoState_.InitialTransform.ScaleY = t.GetWorldScale().y;

                         if (gizmoState_.DraggingType == DragType::Rotate)
                              gizmoState_.InitialDragAngle = std::atan2(screenMouse.y - screenCenter.y, screenMouse.x - screenCenter.x);
                     }
                 }
            }
            else if (gizmoState_.IsDragging)
            {
                float invScale = (std::abs(viewScale.x) > 0.001f) ? (1.0f / viewScale.x) : 1.0f;
                ImVec2 screenDelta = {
                    viewportMousePos.x - gizmoState_.DragStartViewportPos.x,
                    viewportMousePos.y - gizmoState_.DragStartViewportPos.y
                };
                
                Math::Vector2 delta = { screenDelta.x * invScale, screenDelta.y * invScale };
                
                auto t = actor->GetTransform();
                float rad = gizmoState_.InitialTransform.Rot * (std::numbers::pi_v<float> / 180.0f);
                float c = cosf(rad); float s = sinf(rad);
                Math::Vector2 dirX = { c, s }; 
                Math::Vector2 dirY = { -s, c }; 

                if (gizmoState_.DraggingType == DragType::Center)
                {
                    t.SetPosition(Math::Vector3(
                        gizmoState_.InitialTransform.PosX + delta.x,
                        gizmoState_.InitialTransform.PosY + delta.y,
                        t.GetWorldPosition().z
                    ));
                }
                else if (gizmoState_.DraggingType == DragType::XAxis)
                {
                    float proj = delta.x * dirX.x + delta.y * dirX.y;
                    t.SetPosition(Math::Vector3(
                        gizmoState_.InitialTransform.PosX + dirX.x * proj,
                        gizmoState_.InitialTransform.PosY + dirX.y * proj,
                        t.GetWorldPosition().z
                    ));
                }
                else if (gizmoState_.DraggingType == DragType::YAxis)
                {
                    float proj = delta.x * dirY.x + delta.y * dirY.y;
                    t.SetPosition(Math::Vector3(
                        gizmoState_.InitialTransform.PosX + dirY.x * proj,
                        gizmoState_.InitialTransform.PosY + dirY.y * proj,
                        t.GetWorldPosition().z
                    ));
                }
                else if (gizmoState_.DraggingType == DragType::Rotate)
                {
                    float currentAngle = std::atan2(screenMouse.y - screenCenter.y, screenMouse.x - screenCenter.x);
                    float AngleDelta = currentAngle - gizmoState_.InitialDragAngle;
                    
                    Math::Rotator rot = t.GetWorldRotation();
                    rot.yaw = gizmoState_.InitialTransform.Rot + AngleDelta * (180.0f / std::numbers::pi_v<float>);
                    
                    t.SetRotation(rot);
                }
                else if (gizmoState_.DraggingType >= DragType::ScaleTopLeft)
                {
                    Math::Vector2 localDelta = { delta.x * c + delta.y * s, -delta.x * s + delta.y * c };
                    float signX = 1.0f, signY = 1.0f;
                    
                    if (gizmoState_.DraggingType == DragType::ScaleTopLeft)
                    {
                        signX = -1.0f; signY = -1.0f;
                    }
                    else if (gizmoState_.DraggingType == DragType::ScaleTopRight)
                    {
                        signX =  1.0f; signY = -1.0f;
                    }
                    else if (gizmoState_.DraggingType == DragType::ScaleBottomLeft)
                    {
                        signX = -1.0f; signY =  1.0f;
                    }
                    else if (gizmoState_.DraggingType == DragType::ScaleBottomRight)
                    {
                        signX =  1.0f; signY =  1.0f;
                    }

                    float dScaleX = localDelta.x * signX;
                    float dScaleY = localDelta.y * signY;
                    float baseW = 100.0f, baseH = 100.0f;

                    if (auto* sprite = actor->GetComponent<Game::SpriteComponent>())
                    {
                         baseW = sprite->GetWidth(); baseH = sprite->GetHeight();
                    }
                    else if (auto* col = actor->GetComponent<Game::BoxColliderComponent>())
                    {
                         auto ext = col->GetBoxExtent(); baseW = ext.x*2; baseH = ext.y*2;
                    }
                    
                    Math::Vector3 scale = t.GetWorldScale();
                    if (baseW > 1.0f)
                        scale.x = gizmoState_.InitialTransform.ScaleX + dScaleX / (baseW * 0.5f);
                    
                    if (baseH > 1.0f)
                        scale.y = gizmoState_.InitialTransform.ScaleY + dScaleY / (baseH * 0.5f);
                    
                    t.SetScale(scale);
                }
                actor->SetTransform(t);
            }
        }
        else
        {
            gizmoState_.IsDragging = false;
            gizmoState_.DraggingType = DragType::None;
        }
    }

    void SceneViewportPanel::GetActorWorldCorners(Game::Actor* actor, Math::Vector2* outCorners) const
    {
         Math::Matrix3 worldMatrix = actor->GetTransform().ToMatrix3();
         float minX = -25.0f, maxX = 25.0f;
         float minY = -25.0f, maxY = 25.0f;

         if (auto* sprite = actor->GetComponent<Game::SpriteComponent>())
         {
            float w = sprite->GetWidth();
            float h = sprite->GetHeight();
            minX = -w * 0.5f; maxX = w * 0.5f;
            minY = -h * 0.5f; maxY = h * 0.5f;
         }
        else if (auto* col = actor->GetComponent<Game::BoxColliderComponent>())
         {
            auto extent = col->GetBoxExtent();
            minX = -extent.x; maxX = extent.x;
            minY = -extent.y; maxY = extent.y;
         }

         Math::Vector2 corners[4] = {{minX, minY}, {maxX, minY}, {maxX, maxY}, {minX, maxY}};
         for(int i=0; i<4; ++i)
         {
             Math::Vector3 v{corners[i].x, corners[i].y, 1.0f};
             Math::Vector3 res = worldMatrix * v;
             outCorners[i] = {res.x, res.y};
         }
    }

    ImVec2 SceneViewportPanel::WorldToScreen(const Math::Vector2& worldPos, const ImVec2& screenOffset, float viewScale) const
    {
        Math::Vector2 posToConvert = worldPos;
        if (auto* cam = Game::CameraComponent::GetMainCamera())
        {
             Math::Vector3 p3(worldPos.x, worldPos.y, 0.0f);
             posToConvert = cam->WorldToScreen(p3);
             return {
                 screenOffset.x + posToConvert.x * viewScale,
                 screenOffset.y + posToConvert.y * viewScale
             };
        }
        
        return {
            screenOffset.x + worldPos.x * viewScale,
            screenOffset.y + worldPos.y * viewScale
        };
    }

    ImVec2 SceneViewportPanel::RotateVector(const ImVec2& vec, float radians)
    {
        float c = cosf(radians); float s = sinf(radians);
        return {
            vec.x * c - vec.y * s,
            vec.x * s + vec.y * c
        };
    }

    float SceneViewportPanel::DistanceToSegment(const ImVec2& P, const ImVec2& A, const ImVec2& B)
    {
        float l2 = (B.x-A.x)*(B.x-A.x) + (B.y-A.y)*(B.y-A.y);
        
        if (l2 == 0)
            return sqrtf((P.x-A.x)*(P.x-A.x) + (P.y-A.y)*(P.y-A.y));
        
        float t = ((P.x-A.x)*(B.x-A.x) + (P.y-A.y)*(B.y-A.y)) / l2;
        t = (t < 0) ? 0 : (t > 1 ? 1 : t);
        
        ImVec2 proj = {
            A.x + t*(B.x-A.x),
            A.y + t*(B.y-A.y)
        };
        
        return sqrtf((P.x-proj.x)*(P.x-proj.x) + (P.y-proj.y)*(P.y-proj.y));
    }

    void SceneViewportPanel::DrawTranslateGizmo(ImDrawList* drawList, const ImVec2& screenCenter, float rotationRad) const
    {
         const auto& settings = EditorSettings::Get();
         float hs = settings.GizmoHandleSize * 0.5f; 
         float axisLen = settings.GizmoAxisLength;
         float thick = settings.GizmoLineThickness;
         const ImU32 accentColor = ImGui::GetColorU32(settings.ThemeAccentColor);

         drawList->AddRectFilled({screenCenter.x - hs, screenCenter.y - hs}, {screenCenter.x + hs, screenCenter.y + hs}, accentColor);
         
         ImVec2 xDir = RotateVector({1.0f, 0.0f}, rotationRad);
         ImVec2 xEnd = {
             screenCenter.x + xDir.x * axisLen,
             screenCenter.y + xDir.y * axisLen
         };
        
         drawList->AddLine(screenCenter, xEnd, IM_COL32(255, 0, 0, 255), thick);
         
         ImVec2 yDir = RotateVector({0.0f, 1.0f}, rotationRad);
         ImVec2 yEnd = {
             screenCenter.x + yDir.x * axisLen,
             screenCenter.y + yDir.y * axisLen
         };
        
         drawList->AddLine(screenCenter, yEnd, IM_COL32(0, 255, 0, 255), thick);
    }

    void SceneViewportPanel::DrawRotateGizmo(ImDrawList* drawList, const ImVec2& screenCenter, float rotationRad) const
    {
         const auto& settings = EditorSettings::Get();
         float radius = settings.GizmoRotateRadius;
         float thick = settings.GizmoLineThickness;
         const ImU32 accentColor = ImGui::GetColorU32(settings.ThemeAccentColor);

         drawList->AddCircle(screenCenter, radius, accentColor, 32, thick);
         ImVec2 offset = RotateVector({
             0.0f,
             -radius
         }, rotationRad);
        
         ImVec2 knobPos = {
             screenCenter.x + offset.x,
             screenCenter.y + offset.y
         };
        
         drawList->AddLine(screenCenter, knobPos, accentColor, thick);
         drawList->AddCircleFilled(knobPos, 6.0f, accentColor);
    }

    void SceneViewportPanel::DrawScaleGizmo(ImDrawList* drawList, const ImVec2* screenCorners, float rotationRad) const
    {
        const auto& settings = EditorSettings::Get();
        float hs = settings.GizmoHandleSize;
        
        for(int i=0; i<4; ++i)
        {
            ImVec2 center = screenCorners[i];
            ImVec2 p1 = {-hs, -hs}; ImVec2 p2 = { hs, -hs}; ImVec2 p3 = { hs,  hs}; ImVec2 p4 = {-hs,  hs};
            auto Transform = [&](ImVec2 p)
            {
                ImVec2 rot = RotateVector(p, rotationRad);
                return ImVec2(center.x + rot.x, center.y + rot.y);
            };
            
            drawList->AddQuadFilled(Transform(p1), Transform(p2), Transform(p3), Transform(p4), ImGui::GetColorU32(settings.ThemeAccentColor));
            drawList->AddQuad(Transform(p1), Transform(p2), Transform(p3), Transform(p4), IM_COL32_WHITE); 
        }
    }

    SceneViewportPanel::DragType SceneViewportPanel::CheckGizmoHit(const ImVec2& mouseScreen, const ImVec2& screenCenter, const ImVec2* screenCorners, float rotationRad) const
    {
        const auto& settings = EditorSettings::Get();
        float hs = settings.GizmoHandleSize;
        
        auto IsInRect = [&](ImVec2 center)
        {
             return mouseScreen.x >= center.x - hs && mouseScreen.x <= center.x + hs && mouseScreen.y >= center.y - hs && mouseScreen.y <= center.y + hs;
        };

        if (gizmoState_.Mode == GizmoMode::Scale)
        {
             if (IsInRect(screenCorners[0]))
                 return DragType::ScaleTopLeft;
            
             if (IsInRect(screenCorners[1]))
                 return DragType::ScaleTopRight;
            
             if (IsInRect(screenCorners[2]))
                 return DragType::ScaleBottomRight;
            
             if (IsInRect(screenCorners[3]))
                 return DragType::ScaleBottomLeft;
        }
        else if (gizmoState_.Mode == GizmoMode::Rotate)
        {
             ImVec2 offset = RotateVector({0.0f, -settings.GizmoRotateRadius}, rotationRad);
             ImVec2 knobPos = {
                 screenCenter.x + offset.x,
                 screenCenter.y + offset.y
             };
            
             float d2 = (mouseScreen.x - knobPos.x)*(mouseScreen.x - knobPos.x) + (mouseScreen.y - knobPos.y)*(mouseScreen.y - knobPos.y);
             if (d2 <= 100.0f)
                 return DragType::Rotate;
        }
        else if (gizmoState_.Mode == GizmoMode::Translate)
        {
             if (IsInRect(screenCenter))
                 return DragType::Center;
            
             ImVec2 xDir = RotateVector({1.0f, 0.0f}, rotationRad);
             ImVec2 xEnd = {
                 screenCenter.x + xDir.x * settings.GizmoAxisLength,
                 screenCenter.y + xDir.y * settings.GizmoAxisLength
             };
            
             if (DistanceToSegment(mouseScreen, screenCenter, xEnd) <= settings.GizmoSensitivity)
                 return DragType::XAxis;
            
             ImVec2 yDir = RotateVector({0.0f, 1.0f}, rotationRad);
             ImVec2 yEnd = {
                 screenCenter.x + yDir.x * settings.GizmoAxisLength,
                 screenCenter.y + yDir.y * settings.GizmoAxisLength
             };
            
             if (DistanceToSegment(mouseScreen, screenCenter, yEnd) <= settings.GizmoSensitivity)
                 return DragType::YAxis;
        }
        return DragType::None;
    }
}