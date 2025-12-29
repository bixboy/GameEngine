#pragma once
#include <functional>
#include <vector>

namespace BixEngine::Gui::Widgets
{
     
    class PanelToolbar
    {
    public:
        PanelToolbar() = default;
        ~PanelToolbar() = default;

        PanelToolbar(const PanelToolbar&) = delete;
        PanelToolbar& operator=(const PanelToolbar&) = delete;
        PanelToolbar(PanelToolbar&&) = delete;
        PanelToolbar& operator=(PanelToolbar&&) = delete;

        void AddLeft(const std::function<void()>& drawCallback);
        void AddRight(const std::function<void()>& drawCallback);
        void Commit();

    private:
        std::vector<std::function<void()>> leftElements_;
        std::vector<std::function<void()>> rightElements_;
        bool committed_{false};
    };
}
