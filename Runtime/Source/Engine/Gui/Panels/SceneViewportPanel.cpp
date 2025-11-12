#include "Engine/Gui/Panels/SceneViewportPanel.h"

#include <SDL3/SDL_render.h>

#include "Engine/Gui/Internal/GuiManager.h"
#include "Engine/Gui/Internal/GuiPanel.h"
#include "Engine/Gui/Utils/GuiHelpers.h"

#include "imgui.h"
#include "Engine/Gui/Internal/GuiDocking.h"

namespace BixEngine::Gui
{
    using namespace Theme;
    using namespace Utils;

    namespace
    {

        ImVec2 ComputeImageSize(const ImVec2& availableSize, int textureWidth, int textureHeight) noexcept
        {
            if (availableSize.x <= 0.0f || availableSize.y <= 0.0f || textureWidth <= 0 || textureHeight <= 0)
                return ImVec2{0.0f, 0.0f};

            const float textureAspect = static_cast<float>(textureWidth) / static_cast<float>(textureHeight);
            ImVec2 imageSize = availableSize;
            const float availableAspect = availableSize.x / availableSize.y;

            if (availableAspect > textureAspect)
            {
                imageSize.x = availableSize.y * textureAspect;
            }
            else
            {
                imageSize.y = availableSize.x / textureAspect;
            }

            return imageSize;
        }

        void DrawViewportTexture(SDL_Texture* texture, const ImVec2& size, const ImVec2& cursorScreenPos)
        {
            if (!texture || size.x <= 0.0f || size.y <= 0.0f)
                return;

            ImVec2 min = cursorScreenPos;
            ImVec2 max = ImVec2{cursorScreenPos.x + size.x, cursorScreenPos.y + size.y};

            ImDrawList* drawList = ImGui::GetWindowDrawList();
            drawList->AddImage(texture, min, max);
        }
    }

    GuiPanel& CreateSceneViewportPanel(GuiManager& guiManager, const DefaultEngineGuiContext& context)
    {
        GuiPanel& viewportPanel = guiManager.CreatePanel("scene_viewport", "Scene");
        guiManager.SetPanelDockingArea(viewportPanel, DockSpaceRegion::Center, ImGuiCond_FirstUseEver);
        viewportPanel.SetBackgroundColor(ViewportBackground);
        viewportPanel.SetCollapsable(false);
        viewportPanel.SetClosable(false);
        viewportPanel.AddWindowFlags(ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        viewportPanel.SetDrawFunction([
            textureProvider = context.sceneRenderTextureProvider,
            sizeProvider = context.sceneRenderTextureSizeProvider
        ]()
        {
            ScopedID viewportId("SceneViewportPanel");

            const ImVec2 available = ImGui::GetContentRegionAvail();
            if (available.x <= 0.0f || available.y <= 0.0f)
            {
                ImGui::TextUnformatted("Viewport unavailable.");
                return;
            }

            SDL_Texture* texture = textureProvider ? textureProvider() : nullptr;
            std::pair<int, int> size = sizeProvider ? sizeProvider() : std::pair{0, 0};

            if (!texture || size.first <= 0 || size.second <= 0)
            {
                const ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
                ImGui::Dummy(available);
                const ImVec2 textSize = ImGui::CalcTextSize("No scene is currently available.");
                ImVec2 textPos = cursorScreenPos;
                textPos.x += (available.x - textSize.x) * 0.5f;
                textPos.y += (available.y - textSize.y) * 0.5f;
                ImGui::GetWindowDrawList()->AddText(textPos, ImGui::GetColorU32(ImGuiCol_TextDisabled), "No scene is currently available.");
                return;
            }

            const ImVec2 imageSize = ComputeImageSize(available, size.first, size.second);
            const ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
            ImVec2 imagePos = cursorScreenPos;
            imagePos.x += (available.x - imageSize.x) * 0.5f;
            imagePos.y += (available.y - imageSize.y) * 0.5f;

            DrawViewportTexture(texture, imageSize, imagePos);
            ImGui::Dummy(available);

            const ImVec2 infoPos = ImVec2{imagePos.x + 12.0f, imagePos.y + 12.0f};
            const ImU32 infoColor = ImGui::GetColorU32(ImVec4{1.0f, 1.0f, 1.0f, 0.8f});
            ImGui::GetWindowDrawList()->AddText(infoPos, infoColor, "Scene Viewport");

        });

        return viewportPanel;
    }
}

