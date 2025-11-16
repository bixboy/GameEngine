#pragma once

#include <optional>
#include <vector>

#include "Containers/String.h"

namespace BixEngine::Gui
{
    /**
     * @brief Historique simple des panneaux visités avec navigation avant/arrière.
     */
    class PanelHistory
    {
    public:
        void RecordVisit(const String& panelName);

        [[nodiscard]] bool CanGoBack() const noexcept { return !entries_.empty() && currentIndex_ > 0; }
        [[nodiscard]] bool CanGoForward() const noexcept
        {
            return !entries_.empty() && currentIndex_ + 1 < entries_.size();
        }
        [[nodiscard]] bool IsEmpty() const noexcept { return entries_.empty(); }

        [[nodiscard]] const String* NavigateBack();
        [[nodiscard]] const String* NavigateForward();
        [[nodiscard]] const String* NavigateHome();
        [[nodiscard]] const String* Current() const noexcept;

        void Clear();
        void SetHome(const String& panelName);

    private:
        std::vector<String> entries_{};
        std::size_t currentIndex_{0};
        std::optional<String> homePanel_{};
    };
}
