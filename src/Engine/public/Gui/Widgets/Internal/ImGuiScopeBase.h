#pragma once

namespace BixEngine::Gui::Widgets::Internal
{
     
    class ImGuiScopeBase
    {
    public:
        ImGuiScopeBase() = default;
        ImGuiScopeBase(const ImGuiScopeBase&) = delete;
        
        ImGuiScopeBase& operator=(const ImGuiScopeBase&) = delete;
        ImGuiScopeBase(ImGuiScopeBase&&) = delete;
        
        ImGuiScopeBase& operator=(ImGuiScopeBase&&) = delete;
        ~ImGuiScopeBase() = default;

    protected:
         
        void Activate() noexcept { engaged_ = true; }

         
        [[nodiscard]] bool IsActive() const noexcept { return engaged_; }

         
        void Deactivate() noexcept { engaged_ = false; }

    private:
        bool engaged_{false};
    };
}
