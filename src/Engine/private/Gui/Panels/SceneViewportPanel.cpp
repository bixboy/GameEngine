#include "Gui/Panels/SceneViewportPanel.h"
#include "Gui/Core/EditorPreferences.h"
#include <SDL3/SDL_render.h>
#include <utility>

#include "imgui.h"
#include "imgui.h"
#include "Gui/Utils/GuiHelpers.h"
#include "Framework/Actor.h"
#include "Components/Sprite/SpriteComponent.h"
#include "Components/Core/BoxColliderComponent.h"
#include "Components/Core/CameraComponent.h"
#include "Serializer/SceneSerializer.h"
#include "Utils/FileIO/BinaryUtils.h"
#include "Utils/FileIO/PrefabUtils.h"
#include "Debug/Logger.h"
#include <filesystem>
#include <fstream>
#include "Math/Rect.h" // Keep existing include

namespace fs = std::filesystem;

// COLOR_SELECTION removed, now using EditorSettings::Get().ThemeAccentColor


namespace BixEngine::Gui
{
    using namespace Utils;

    namespace
    {
        // Helper to load actor from binary (Duplicated from ActorEditorController for now)
        std::unique_ptr<Game::Actor> LoadActorBinary(const std::filesystem::path& path)
        {
            std::ifstream file(path, std::ios::binary);
            if (file.is_open())
            {
                BixEngine::Utils::BinaryReader reader(file);
                
                String typeName;
                std::unique_ptr<Game::Actor> actor = nullptr;

                // Try reading type name first
                if (reader.ReadString(typeName) && !typeName.IsEmpty())
                {
                    actor = BixEngine::Serialization::SceneSerializer::CreateActor(typeName);
                    if (!actor)
                    {
                        LOG_WARNING("LoadActorBinary: Could not create actor of type " + typeName + ". Fallback to Game::Actor.");
                        actor = std::make_unique<Game::Actor>("Root");
                    }
                }
                else
                {
                    file.clear();
                    file.seekg(0, std::ios::beg); // Reset if read failed
                    actor = std::make_unique<Game::Actor>("Root");
                }

                try
                {
                    // Load data
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
            if (availableSize.x <= 0.0f || availableSize.y <= 0.0f || textureWidth <= 0 || textureWidth <= 0)
                return ImVec2{0,0};

            const float texAspect = static_cast<float>(textureWidth) / textureHeight;
            const float availAspect = availableSize.x / availableSize.y;

            ImVec2 final = availableSize;
            if (availAspect > texAspect)
                final.x = final.y * texAspect;
            else
                final.y = final.x / texAspect;

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
    
    void SceneViewportPanel::Draw()
    {
        ScopedID id("SceneViewport");

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

            ImGui::GetWindowDrawList()->AddText(
                center,
                ImGui::GetColorU32(ImGuiCol_TextDisabled),
                "No scene is currently available."
            );
            
            return;
        }

        const ImVec2 imgSize = ComputeImageSize(avail, size.first, size.second);
        ImVec2 drawPos = cursor;

        drawPos.x += (avail.x - imgSize.x) * 0.5f;
        drawPos.y += (avail.y - imgSize.y) * 0.5f;

        DrawTexture(texture, drawPos, imgSize);
        ImGui::Dummy(avail);

        ImVec2 overlayPos = { drawPos.x + 12.f, drawPos.y + 12.f };
        ImGui::GetWindowDrawList()->AddText(overlayPos, ImGui::GetColorU32(ImVec4(1,1,1,0.8f)), "Scene Viewport");

        // --- Drag & Drop Target ---
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
            {
                std::filesystem::path path((const char*)payload->Data);
                if (path.extension() == ".bixactor")
                {
                    // Use PrefabSerializer to support V2 hierarchy
                    auto root = PrefabUtils::PrefabSerializer::LoadPrefab(path);
                    if (root)
                    {
                        Game::Scene* scene = nullptr;
                        if (context_.sceneProvider) scene = context_.sceneProvider();
                        else if (context_.sceneManagerProvider && context_.sceneManagerProvider()) 
                            scene = context_.sceneManagerProvider()->GetActiveScene();

                        if (scene)
                        {
                            // 1. Snapshot Hierarchy (because Scene::AddActor resets parents)
                            struct HierarchyNode { Game::Actor* actor; Game::Actor* parent; };
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

                            // 2. Add Root to Scene
                            Game::Actor* rootPtr = root.get();
                            scene->AddActor(std::move(root));

                            // 3. Add Descendants to Scene (Transfer ownership)
                            for(auto* child : descendants)
                            {
                                // Re-acquire ownership (was released in LoadPrefab)
                                std::unique_ptr<Game::Actor> uChild(child);
                                scene->AddActor(std::move(uChild));
                            }

                            // 4. Restore Hierarchy
                            for(const auto& node : hierarchy)
                            {
                                if (node.actor && node.parent)
                                    node.actor->SetParent(node.parent);
                            }

                            // 5. Select Root
                             if (context_.selectedActorSetter)
                                context_.selectedActorSetter(rootPtr);

                            LOG_INFO("Added Prefab to Scene: " + path.generic_string());
                        }
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }

        // --- GIZMO & SELECTION Logic ---
        // 1. Determine Scale and Offset (Texture Space -> Screen Space)
        // Texture Space: (0,0) to (size.first, size.second)
        // Screen Space: drawPos to drawPos + imgSize
        // We assume World Space == Texture Space (Pixel for Pixel)
        
        float scaleX = imgSize.x / (float)size.first;
        float scaleY = imgSize.y / (float)size.second;
        // Assuming uniform scale for simplicity in keeping aspect ratio generally
        // But ComputeImageSize preserves aspect ratio, so scaleX ~= scaleY usually.
        
        Game::Actor* selectedActor = context_.selectedActorGetter ? context_.selectedActorGetter() : nullptr;

        // Interaction
        // Priority 1: Gizmo Interaction
        if (selectedActor)
        {
            // Draw Gizmo
            DrawGizmo(selectedActor, drawPos, scaleX);
            
             // Handle Gizmo Logic
            ImVec2 mousePos = ImGui::GetMousePos();
            // Pass View Scale (Assuming uniform X for now, or pass both)
            HandleGizmoInteraction(selectedActor, drawPos, mousePos, {scaleX, scaleY});
        }

        // Priority 2: Scene Selection
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0))
        {
             ImVec2 mousePos = ImGui::GetMousePos();
             // Convert to Viewport (World) Space
             ImVec2 localMouse = { (mousePos.x - drawPos.x) / scaleX, (mousePos.y - drawPos.y) / scaleY };
             
            if (localMouse.x >= 0 && localMouse.x <= size.first && localMouse.y >= 0 && localMouse.y <= size.second)
            {
                 // Handle Selection (Only if not already dragging a gizmo)
                 if (!gizmoState_.IsDragging)
                 {
                     HandleSelection(localMouse);
                     selectedActor = context_.selectedActorGetter ? context_.selectedActorGetter() : nullptr; // Refresh
                 }
            }
        }

        // Toolbar Overlay (Mode Selection)
        ImGui::SetCursorScreenPos({cursor.x + 10, cursor.y + 40});
        if (ImGui::Button("Translate")) gizmoState_.Mode = GizmoMode::Translate;
        ImGui::SameLine();
        if (ImGui::Button("Rotate")) gizmoState_.Mode = GizmoMode::Rotate;
        ImGui::SameLine();
        if (ImGui::Button("Scale")) gizmoState_.Mode = GizmoMode::Scale;
    }

    void SceneViewportPanel::HandleSelection(const ImVec2& viewportMousePos)
    {
         Game::Scene* scene = nullptr;
         if (context_.sceneProvider)
         {
             scene = context_.sceneProvider();
         }
         else if (context_.sceneManagerProvider)
         {
             if (auto* sm = context_.sceneManagerProvider())
                 scene = sm->GetActiveScene();
         }

         if (!scene) return;
         
         const auto& actors = scene->GetActors();
         
         // Get Camera if active
         Game::CameraComponent* cam = Game::CameraComponent::GetMainCamera();

         for (auto it = actors.rbegin(); it != actors.rend(); ++it)
         {
             Game::Actor* actor = it->get();
             if (!actor) continue;

             // Use Helper
             Math::Vector2<float> worldCorners[4];
             GetActorWorldCorners(actor, worldCorners);
                
             // If Camera is active, transform World Corners to Screen Space (Pixel Space)
             if (cam)
             {
                 for(int i=0; i<4; ++i)
                 {
                      Math::Vector3 p3(worldCorners[i].x, worldCorners[i].y, 0.0f);
                      Math::Vector2<float> screenPos = cam->WorldToScreen(p3);
                      worldCorners[i] = screenPos;
                 }
             }

             // Compute AABB of the (possibly projected) corners for picking
             float wMinX = worldCorners[0].x, wMaxX = worldCorners[0].x;
             float wMinY = worldCorners[0].y, wMaxY = worldCorners[0].y;

             for(int i=1; i<4; ++i)
             {
                 if(worldCorners[i].x < wMinX) wMinX = worldCorners[i].x;
                 if(worldCorners[i].x > wMaxX) wMaxX = worldCorners[i].x;
                 if(worldCorners[i].y < wMinY) wMinY = worldCorners[i].y;
                 if(worldCorners[i].y > wMaxY) wMaxY = worldCorners[i].y;
             }
             
             if (viewportMousePos.x >= wMinX && viewportMousePos.x <= wMaxX &&
                 viewportMousePos.y >= wMinY && viewportMousePos.y <= wMaxY)
             {
                 if (context_.selectedActorSetter)
                    context_.selectedActorSetter(actor);
                 return;
             }
         }

         // Deselect if clicking on empty space
         if (context_.selectedActorSetter)
             context_.selectedActorSetter(nullptr);
    }

    void SceneViewportPanel::DrawGizmo(Game::Actor* actor, const ImVec2& screenOffset, float viewScale)
    {
        Math::Vector2<float> worldCorners[4];
        GetActorWorldCorners(actor, worldCorners);

        ImVec2 screenCorners[4];
        for(int i=0; i<4; ++i)
        {
            screenCorners[i] = WorldToScreen(worldCorners[i], screenOffset, viewScale);
        }

        auto pos = actor->GetTransform().GetWorldPosition(); 
        ImVec2 screenCenter = WorldToScreen({pos.x, pos.y}, screenOffset, viewScale);
        
        float rotationRad = actor->GetTransform().rotation.yaw * (3.1415926535f / 180.0f);

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // Draw OBB
        const ImU32 accentColor = ImGui::GetColorU32(EditorSettings::Get().ThemeAccentColor);
        drawList->AddQuad(screenCorners[0], screenCorners[1], screenCorners[2], screenCorners[3], accentColor, 2.0f);

        // Delegate Drawing
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
        float rotationRad = actor->GetTransform().rotation.yaw * (3.1415926535f / 180.0f);

        ImVec2 screenCenter = WorldToScreen({pos.x, pos.y}, screenOffset, viewScale.x); // Assuming uniform scale X/Y for viewport rendering logic?
        // viewScale is a pair, let's use viewScale based on previous code logic (passed as scaleX, scaleY but functionally uniform usually)

        Math::Vector2<float> worldCorners[4];
        GetActorWorldCorners(actor, worldCorners);
        
        ImVec2 screenCorners[4];
        for(int i=0; i<4; ++i)
        {
            screenCorners[i] = WorldToScreen(worldCorners[i], screenOffset, viewScale.x);
        }

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
                         gizmoState_.InitialTransform.PosX = t.position.x;
                         gizmoState_.InitialTransform.PosY = t.position.y;
                         gizmoState_.InitialTransform.Rot = t.rotation.yaw;
                         gizmoState_.InitialTransform.ScaleX = t.scale.x;
                         gizmoState_.InitialTransform.ScaleY = t.scale.y;

                         if (gizmoState_.DraggingType == DragType::Rotate)
                         {
                              gizmoState_.InitialDragAngle = std::atan2(screenMouse.y - screenCenter.y, screenMouse.x - screenCenter.x);
                         }
                     }
                 }
            }
            else if (gizmoState_.IsDragging)
            {
                // Convert Screen Pixel Delta to World Unit Delta
                // viewScale.x is PixelsPerUnit
                float invScale = (std::abs(viewScale.x) > 0.001f) ? (1.0f / viewScale.x) : 1.0f;
                
                ImVec2 screenDelta = { viewportMousePos.x - gizmoState_.DragStartViewportPos.x, viewportMousePos.y - gizmoState_.DragStartViewportPos.y };
                Math::Vector2<float> delta = { screenDelta.x * invScale, screenDelta.y * invScale };
                
                auto t = actor->GetTransform();

                // Calculate Rotated Axes
                float rad = gizmoState_.InitialTransform.Rot * (3.1415926535f / 180.0f);
                float c = cosf(rad);
                float s = sinf(rad);
                Math::Vector2<float> dirX = { c, s }; // Local Right
                Math::Vector2<float> dirY = { -s, c }; // Local Down (90 deg)

                if (gizmoState_.DraggingType == DragType::Center)
                {
                    // Global Translate
                    t.position.x = gizmoState_.InitialTransform.PosX + delta.x;
                    t.position.y = gizmoState_.InitialTransform.PosY + delta.y;
                }
                else if (gizmoState_.DraggingType == DragType::XAxis)
                {
                    // Local X Translate
                    // Project delta onto DirX
                    float proj = delta.x * dirX.x + delta.y * dirX.y;
                    t.position.x = gizmoState_.InitialTransform.PosX + dirX.x * proj;
                    t.position.y = gizmoState_.InitialTransform.PosY + dirX.y * proj;
                }
                else if (gizmoState_.DraggingType == DragType::YAxis)
                {
                    // Local Y Translate
                    // Project delta onto DirY
                    float proj = delta.x * dirY.x + delta.y * dirY.y;
                    t.position.x = gizmoState_.InitialTransform.PosX + dirY.x * proj;
                    t.position.y = gizmoState_.InitialTransform.PosY + dirY.y * proj;
                }
                else if (gizmoState_.DraggingType == DragType::Rotate)
                {
                    float currentAngle = std::atan2(screenMouse.y - screenCenter.y, screenMouse.x - screenCenter.x);
                    float AngleDelta = currentAngle - gizmoState_.InitialDragAngle;
                    float AngleDeltaDeg = AngleDelta * (180.0f / 3.1415926535f);
                    t.rotation.yaw = gizmoState_.InitialTransform.Rot + AngleDeltaDeg;
                }
                else if (gizmoState_.DraggingType >= DragType::ScaleTopLeft)
                {
                    // Linear Scaling
                    // Project delta onto local axes
                    
                    // We need the "Local Delta"
                    Math::Vector2<float> localDelta = { 
                        delta.x * c + delta.y * s,   // Rotate Delta by -Rad (Inverse)
                        -delta.x * s + delta.y * c 
                    };

                    // Depending on corner, the "Direction" of growth changes
                    // TL: (-1, -1) -> Dragging Left/Up (-,-) increases Size. 
                    // To make it intuitive: Dragging *Away* from Center increases size.
                    
                    // Let's look at the signs of the Corner relative to Center
                    float signX = 1.0f;
                    float signY = 1.0f;
                    
                    if (gizmoState_.DraggingType == DragType::ScaleTopLeft)      { signX = -1.0f; signY = -1.0f; }
                    else if (gizmoState_.DraggingType == DragType::ScaleTopRight)   { signX =  1.0f; signY = -1.0f; }
                    else if (gizmoState_.DraggingType == DragType::ScaleBottomLeft) { signX = -1.0f; signY =  1.0f; }
                    else if (gizmoState_.DraggingType == DragType::ScaleBottomRight){ signX =  1.0f; signY =  1.0f; }

                    // If I drag "Right" (Positive X) on TopRight (Positive X Corner), I expect Grow.
                    // If I drag "Left" (Negative X) on TopLeft (Negative X Corner), I expect Grow.
                    // So GrowthDelta = LocalDelta * Sign.
                    
                    float dScaleX = localDelta.x * signX;
                    float dScaleY = localDelta.y * signY;

                    // Now we need to normalize this delta by the Object Size to get Scale factor?
                    // Or just apply it to Scale directly?
                    // If Scale=1 means 100 pixels, and I drag 100 pixels, I expect Scale=2.
                    // So dScale needs to be divided by "Base Size".
                    
                    float baseW = 100.0f; // Default fallback
                    float baseH = 100.0f;

                    if (auto* sprite = actor->GetComponent<Game::SpriteComponent>()) {
                         baseW = sprite->GetWidth(); baseH = sprite->GetHeight();
                    } else if (auto* col = actor->GetComponent<Game::BoxColliderComponent>()) {
                         auto ext = col->GetBoxExtent(); baseW = ext.x*2; baseH = ext.y*2;
                    }
                    
                    // Factor in the Initial Scale to keep it proportional?
                    // No, `Scale = (CurrentSize) / BaseSize`.
                    // `CurrentSize = InitialSize + Delta`.
                    // `InitialSize = BaseSize * InitialScale`.
                    // `Scale = (BaseSize * InitialScale + Delta) / BaseSize`
                    // `Scale = InitialScale + Delta / BaseSize`.
                    
                    if (baseW > 0.1f) t.scale.x = gizmoState_.InitialTransform.ScaleX + (dScaleX / baseW) * 2.0f; // *2 because visual handles are on half-extents? No.
                    // Wait, My handles are at W/2, H/2.
                    // If I drag 10 units away, I increase Half-Width by 10?
                    // Total Width increases by 20? 
                    // If I treat Center as anchor.
                    // Yes, Scaling around Center means dragging corner 10px moves that corner 10px.
                    // So Half-Size increases by 10px.
                    // Total Size increases by 20px.
                    // So `DeltaScale = (Delta * 2) / BaseSize`.
                    // Actually, let's just stick to `Delta / (BaseSize / 2)` which is same.
                    
                    if (baseW > 1.0f) t.scale.x = gizmoState_.InitialTransform.ScaleX + dScaleX / (baseW * 0.5f);
                    if (baseH > 1.0f) t.scale.y = gizmoState_.InitialTransform.ScaleY + dScaleY / (baseH * 0.5f);
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

    // --- Helpers Wrappers ---

    void SceneViewportPanel::GetActorWorldCorners(Game::Actor* actor, Math::Vector2<float>* outCorners) const
    {
         Math::Matrix3 worldMatrix = actor->GetTransform().ToMatrix3();
         
         float minX = -25.0f, maxX = 25.0f;
         float minY = -25.0f, maxY = 25.0f;

         if (auto* sprite = actor->GetComponent<Game::SpriteComponent>())
         {
             float w = sprite->GetWidth();
             float h = sprite->GetHeight();
             // Assuming Centered Pivot for bounds consistency
             minX = -w * 0.5f; maxX = w * 0.5f;
             minY = -h * 0.5f; maxY = h * 0.5f;
         }
         else if (auto* col = actor->GetComponent<Game::BoxColliderComponent>())
         {
             auto extent = col->GetBoxExtent();
             minX = -extent.x; maxX = extent.x;
             minY = -extent.y; maxY = extent.y;
         }

         Math::Vector2<float> corners[4] = {
            {minX, minY}, {maxX, minY}, {maxX, maxY}, {minX, maxY}
         };

         for(int i=0; i<4; ++i)
         {
             Math::Vector3 v{corners[i].x, corners[i].y, 1.0f};
             Math::Vector3 res = worldMatrix * v;
             outCorners[i] = {res.x, res.y};
         }
    }

    ImVec2 SceneViewportPanel::WorldToScreen(const Math::Vector2<float>& worldPos, const ImVec2& screenOffset, float viewScale) const
    {
        Math::Vector2<float> posToConvert = worldPos;
        
        // If there is an active game camera, the viewport (which renders the scene) is already shifted by it.
        // So we must apply the same shift to the gizmos that we draw ON TOP of that scene.
        if (auto* cam = Game::CameraComponent::GetMainCamera())
        {
             // Camera WorldToScreen transforms World -> Screen (Pixels)
             // But here we are in "Viewport Texture Space".
             // We need to match what SpriteComponent does.
             
             // SpriteComponent: final = cam->WorldToScreen(world)
             // This returns coordinates in the logical screen resolution.
             
             Math::Vector3 p3(worldPos.x, worldPos.y, 0.0f);
             posToConvert = cam->WorldToScreen(p3);
             
             // Now 'posToConvert' is in "Screen Pixel Coords" (e.g. 0 to 1600).
             // BUT, SceneViewportPanel draws into an ImGui window that might be scaled.
             // 'screenOffset' corresponds to the top-left of the image in ImGui window.
             // 'viewScale' relates the Texture Size to the Display Size.
             
             // If Camera is active, the Texture content ITSELF is already transformed.
             // So (0,0) in the Texture is actually (0,0) on the Screen.
             // Wait.
             // The Sprite at 'posToConvert' (e.g. 800, 450) is drawn at pixel (800, 450) in the Texture.
             // So we just need to map Texture Pixel (800, 450) to ImGui Screen Coord.
             
             // SceneViewportPanel Logic:
             // DrawTexture(texture, drawPos, imgSize)
             // scaleX = imgSize.x / textureWidth.
             
             // So Output = drawPos + (TextureCoord * Scale).
             
             return { screenOffset.x + posToConvert.x * viewScale, screenOffset.y + posToConvert.y * viewScale };
        }

        // Default behavior (No Camera)
        return { screenOffset.x + worldPos.x * viewScale, screenOffset.y + worldPos.y * viewScale };
    }

    ImVec2 SceneViewportPanel::RotateVector(const ImVec2& vec, float radians)
    {
        float c = cosf(radians);
        float s = sinf(radians);
        return { vec.x * c - vec.y * s, vec.x * s + vec.y * c };
    }

    float SceneViewportPanel::DistanceToSegment(const ImVec2& P, const ImVec2& A, const ImVec2& B)
    {
        float l2 = (B.x-A.x)*(B.x-A.x) + (B.y-A.y)*(B.y-A.y);
        if (l2 == 0) return sqrtf((P.x-A.x)*(P.x-A.x) + (P.y-A.y)*(P.y-A.y));
        float t = ((P.x-A.x)*(B.x-A.x) + (P.y-A.y)*(B.y-A.y)) / l2;
        t = (t < 0) ? 0 : (t > 1 ? 1 : t);
        ImVec2 proj = { A.x + t*(B.x-A.x), A.y + t*(B.y-A.y) };
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
         
         // X Axis (Red)
         ImVec2 xDir = RotateVector({1.0f, 0.0f}, rotationRad);
         ImVec2 xEnd = { screenCenter.x + xDir.x * axisLen, screenCenter.y + xDir.y * axisLen };
         drawList->AddLine(screenCenter, xEnd, IM_COL32(255, 0, 0, 255), thick);
         
         // Y Axis (Green)
         ImVec2 yDir = RotateVector({0.0f, 1.0f}, rotationRad);
         ImVec2 yEnd = { screenCenter.x + yDir.x * axisLen, screenCenter.y + yDir.y * axisLen };
         drawList->AddLine(screenCenter, yEnd, IM_COL32(0, 255, 0, 255), thick);
    }

    void SceneViewportPanel::DrawRotateGizmo(ImDrawList* drawList, const ImVec2& screenCenter, float rotationRad) const
    {
         const auto& settings = EditorSettings::Get();
         float radius = settings.GizmoRotateRadius;
         float thick = settings.GizmoLineThickness;
         const ImU32 accentColor = ImGui::GetColorU32(settings.ThemeAccentColor);

         drawList->AddCircle(screenCenter, radius, accentColor, 32, thick);
         
         ImVec2 offset = RotateVector({0.0f, -radius}, rotationRad);
         ImVec2 knobPos = { screenCenter.x + offset.x, screenCenter.y + offset.y };

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
            ImVec2 p1 = {-hs, -hs}; ImVec2 p2 = { hs, -hs};
            ImVec2 p3 = { hs,  hs}; ImVec2 p4 = {-hs,  hs};

            auto Transform = [&](ImVec2 p) {
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

        auto IsInRect = [&](ImVec2 center) {
             return mouseScreen.x >= center.x - hs && mouseScreen.x <= center.x + hs &&
                    mouseScreen.y >= center.y - hs && mouseScreen.y <= center.y + hs;
        };

        if (gizmoState_.Mode == GizmoMode::Scale)
        {
             if (IsInRect(screenCorners[0])) return DragType::ScaleTopLeft;
             if (IsInRect(screenCorners[1])) return DragType::ScaleTopRight;
             if (IsInRect(screenCorners[2])) return DragType::ScaleBottomRight;
             if (IsInRect(screenCorners[3])) return DragType::ScaleBottomLeft;
        }
        else if (gizmoState_.Mode == GizmoMode::Rotate)
        {
             ImVec2 offset = RotateVector({0.0f, -settings.GizmoRotateRadius}, rotationRad);
             ImVec2 knobPos = { screenCenter.x + offset.x, screenCenter.y + offset.y };
             
             float d2 = (mouseScreen.x - knobPos.x)*(mouseScreen.x - knobPos.x) + (mouseScreen.y - knobPos.y)*(mouseScreen.y - knobPos.y);
             if (d2 <= 100.0f) return DragType::Rotate;
        }
        else if (gizmoState_.Mode == GizmoMode::Translate)
        {
             // Center
             if (IsInRect(screenCenter)) return DragType::Center;
             
             ImVec2 xDir = RotateVector({1.0f, 0.0f}, rotationRad);
             ImVec2 xEnd = { screenCenter.x + xDir.x * settings.GizmoAxisLength, screenCenter.y + xDir.y * settings.GizmoAxisLength };
             if (DistanceToSegment(mouseScreen, screenCenter, xEnd) <= settings.GizmoSensitivity) return DragType::XAxis;

             ImVec2 yDir = RotateVector({0.0f, 1.0f}, rotationRad);
             ImVec2 yEnd = { screenCenter.x + yDir.x * settings.GizmoAxisLength, screenCenter.y + yDir.y * settings.GizmoAxisLength };
             if (DistanceToSegment(mouseScreen, screenCenter, yEnd) <= settings.GizmoSensitivity) return DragType::YAxis;
        }

        return DragType::None;
    }

}
