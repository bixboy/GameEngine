#pragma once
#include <string>
#include "imgui.h"

namespace BixEngine::Gui::Widgets
{
     
    class PersistentSectionScope
    {
    public:
        PersistentSectionScope(const char* label, const std::string& contextId, bool defaultOpen = true,
                               ImGuiTreeNodeFlags flags = 0);
        ~PersistentSectionScope();

        PersistentSectionScope(const PersistentSectionScope&) = delete;
        PersistentSectionScope& operator=(const PersistentSectionScope&) = delete;
        PersistentSectionScope(PersistentSectionScope&&) = delete;
        PersistentSectionScope& operator=(PersistentSectionScope&&) = delete;

        [[nodiscard]] bool IsOpen() const noexcept { return isOpen_; }

    private:
        bool isOpen_{false};
    };
}
