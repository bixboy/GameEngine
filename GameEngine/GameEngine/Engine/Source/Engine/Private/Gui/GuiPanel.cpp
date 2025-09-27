#include "Gui/GuiPanel.h"

#include <utility>

namespace Engine::Gui
{
    GuiPanel::GuiPanel(std::string name, std::string title)
        : name_(std::move(name)), title_(std::move(title))
    {
    }

    void GuiPanel::SetTitle(std::string title)
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

    void GuiPanel::Draw()
    {
        if (!visible_)
            return;

        if (usePosition_)
            ImGui::SetNextWindowPos(position_, positionCondition_);

        if (useSize_)
            ImGui::SetNextWindowSize(size_, sizeCondition_);

        if (useBackgroundColor_)
            ImGui::PushStyleColor(ImGuiCol_WindowBg, backgroundColor_);

        ImGuiWindowFlags finalFlags = windowFlags_;
        if (!resizable_)
            finalFlags |= ImGuiWindowFlags_NoResize;
        if (!movable_)
            finalFlags |= ImGuiWindowFlags_NoMove;

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
