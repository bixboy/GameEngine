#include "Gui/GuiPanel.h"

#include <utility>

namespace Engine::Gui
{
    GuiPanel::GuiPanel(String name, String title)
        : name_(std::move(name)), title_(std::move(title))
    {
    }

    void GuiPanel::SetTitle(String title)
    {
        title_ = std::move(title);
    }

    void GuiPanel::SetPosition(float x, float y, ImGuiCond condition) noexcept
    {
        usePosition_ = true;
        position_ = ImVec2{x, y};
        positionCondition_ = condition;
    }

    void GuiPanel::SetSize(float width, float height, ImGuiCond condition) noexcept
    {
        useSize_ = true;
        size_ = ImVec2{width, height};
        sizeCondition_ = condition;
    }

    void GuiPanel::SetBackgroundColor(const ImVec4& color) noexcept
    {
        backgroundColor_ = color;
        useBackgroundColor_ = true;
    }

    void GuiPanel::SetDrawFunction(DrawFunction drawFunction)
    {
        drawFunction_ = std::move(drawFunction);
    }

    void GuiPanel::SetDockingPreference(DockSpaceRegion area, ImGuiCond condition) noexcept
    {
        dockPreferenceSet_ = true;
        dockPreference_ = area;
        dockPreferenceCondition_ = condition;
        ResetDockId();
    }

    void GuiPanel::ResetDockingPreference() noexcept
    {
        dockPreferenceSet_ = false;
        dockPreference_ = DockSpaceRegion::Center;
        dockPreferenceCondition_ = ImGuiCond_FirstUseEver;
        ResetDockId();
    }

    void GuiPanel::SetDockId(ImGuiID dockId, ImGuiCond condition, ImGuiCond fallbackCondition) noexcept
    {
        dockId_ = dockId;
        dockCondition_ = condition;
        dockFallbackCondition_ = fallbackCondition;
        useDockId_ = dockId_ != 0;
        applyDockFallback_ = useDockId_ && dockCondition_ == ImGuiCond_Always && dockCondition_ != dockFallbackCondition_;
    }

    void GuiPanel::ResetDockId() noexcept
    {
        useDockId_ = false;
        applyDockFallback_ = false;
        dockId_ = 0;
        dockCondition_ = ImGuiCond_FirstUseEver;
        dockFallbackCondition_ = ImGuiCond_FirstUseEver;
    }

    void GuiPanel::Draw()
    {
        if (!visible_)
            return;

        if (usePosition_)
            ImGui::SetNextWindowPos(position_, positionCondition_);

        if (useSize_)
            ImGui::SetNextWindowSize(size_, sizeCondition_);

        if (useDockId_ && dockId_ != 0)
        {
            ImGui::SetNextWindowDockID(dockId_, dockCondition_);

            if (applyDockFallback_)
            {
                dockCondition_ = dockFallbackCondition_;
                applyDockFallback_ = false;
                if (dockFallbackCondition_ == ImGuiCond_None)
                    useDockId_ = false;
            }
        }

        if (useBackgroundColor_)
            ImGui::PushStyleColor(ImGuiCol_WindowBg, backgroundColor_);

        ImGuiWindowFlags finalFlags = windowFlags_;

        if (!resizable_)
            finalFlags |= ImGuiWindowFlags_NoResize;
        
        if (!movable_)
            finalFlags |= ImGuiWindowFlags_NoMove;

        if (!collapsable_)
            finalFlags |= ImGuiWindowFlags_NoCollapse;
        else
            finalFlags &= ~ImGuiWindowFlags_NoCollapse;

        bool open = visible_;
        const bool shouldShow = closable_
            ? ImGui::Begin(title_.c_str(), &open, finalFlags)
            : ImGui::Begin(title_.c_str(), nullptr, finalFlags);

        if (closable_)
            visible_ = open;
        
        if (shouldShow && drawFunction_)
            drawFunction_();

        ImGui::End();

        if (useBackgroundColor_)
            ImGui::PopStyleColor();
    }
}
