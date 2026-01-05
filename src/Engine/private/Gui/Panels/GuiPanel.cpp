#include "Gui/Panels/GuiPanel.h"
#include "Debug/Logger.h"
#include <utility>
#include "Gui/Core/GuiData.h"


namespace BixEngine::Gui
{
    GuiPanel::GuiPanel(String name, String title) : name_(std::move(name)), title_(std::move(title))
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

    void GuiPanel::SetSizeConstraints(const ImVec2& minSize, const ImVec2& maxSize) noexcept
    {
        minSize_ = minSize;
        maxSize_ = maxSize;
        useConstraints_ = true;
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

    bool GuiPanel::IsFocused() const noexcept
    {
        return ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    }

    bool GuiPanel::IsHovered() const noexcept
    {
        return ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
    }

    void GuiPanel::Draw()
    {
        if (!visible_)
            return;

        SetupWindow();

        ImGuiWindowFlags finalFlags = windowFlags_;

        if (!resizable_)
            finalFlags |= ImGuiWindowFlags_NoResize;
        
        if (!movable_)
            finalFlags |= ImGuiWindowFlags_NoMove;

        if (!collapsable_)
        {
            finalFlags |= ImGuiWindowFlags_NoCollapse;
        }
        else
        {
            finalFlags &= ~ImGuiWindowFlags_NoCollapse;
        }

        bool wasVisible = visible_;
        bool open = visible_;

        bool shouldShow = closable_ ? ImGui::Begin(title_.c_str(), &open, finalFlags) : ImGui::Begin(title_.c_str(), nullptr, finalFlags);

        if (!wasVisible && open && OnOpen)
            OnOpen();
        
        if (wasVisible && !open && OnClose)
            OnClose();

        visible_ = open;

        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
        {
            if (OnFocus)
                OnFocus();
        }
        else
        {
            if (OnUnfocus)
                OnUnfocus();
        }

        windowPos_ = ImGui::GetWindowPos();
        windowSize_ = ImGui::GetWindowSize();

        try
        {
            if (shouldShow && drawFunction_)
                drawFunction_();
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(String{"GuiPanel "} + name_ + " draw exception: " + e.what());
        }

        if (contextMenu_ && ImGui::BeginPopupContextWindow())
        {
            contextMenu_();
            ImGui::EndPopup();
        }

        ImGui::End();

        TeardownWindow();
    }

    void GuiPanel::SetupWindow()
    {
        ImGui::PushID(name_.c_str());

        if (requestFocus_)
        {
            ImGui::SetNextWindowFocus();
            requestFocus_ = false;
        }

        if (useConstraints_)
        {
            ImGui::SetNextWindowSizeConstraints(minSize_, maxSize_);
        }

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

        stylePushedInCurrentFrame_ = style_.override;
        colorPushedInCurrentFrame_ = useBackgroundColor_ && !style_.override;

        if (stylePushedInCurrentFrame_)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, style_.rounding);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, style_.border);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, style_.bgColor);
        }
        else if (colorPushedInCurrentFrame_)
        {
            ImGui::PushStyleColor(ImGuiCol_WindowBg, backgroundColor_);
        }
    }

    void GuiPanel::TeardownWindow()
    {
        if (stylePushedInCurrentFrame_)
        {
            ImGui::PopStyleColor();
            ImGui::PopStyleVar(2);
        }
        else if (colorPushedInCurrentFrame_)
        {
            ImGui::PopStyleColor();
        }

        stylePushedInCurrentFrame_ = false;
        colorPushedInCurrentFrame_ = false;

        ImGui::PopID();
    }
}