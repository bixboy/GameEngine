#include "Gui/Panels/SceneViewportPanel.h"
#include <SDL3/SDL_render.h>
#include <utility>

#include "imgui.h"
#include "Gui/Utils/GuiHelpers.h"


namespace BixEngine::Gui
{
    using namespace Utils;

    namespace
    {
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
        ImGui::GetWindowDrawList()->AddText(overlayPos, ImGui::GetColorU32(ImVec4(1,1,1,0.8f)), "Scene Viewport"
        );
    }

}
