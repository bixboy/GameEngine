#include "Gui/Internal/PanelHistory.h"

#include <algorithm>
#include <cstddef>

namespace BixEngine::Gui
{
    void PanelHistory::RecordVisit(const String& panelName)
    {
        if (panelName.empty())
            return;

        if (!entries_.empty())
        {
            if (entries_[currentIndex_] == panelName)
                return;

            if (currentIndex_ + 1 < entries_.size())
                entries_.erase(entries_.begin() + static_cast<std::ptrdiff_t>(currentIndex_ + 1), entries_.end());
        }

        entries_.push_back(panelName);
        currentIndex_ = entries_.size() - 1;

        if (!homePanel_.has_value())
            homePanel_ = panelName;
    }

    const String* PanelHistory::NavigateBack()
    {
        if (!CanGoBack())
            return nullptr;

        --currentIndex_;
        return &entries_[currentIndex_];
    }

    const String* PanelHistory::NavigateForward()
    {
        if (!CanGoForward())
            return nullptr;

        ++currentIndex_;
        return &entries_[currentIndex_];
    }

    const String* PanelHistory::NavigateHome()
    {
        if (entries_.empty())
            return nullptr;

        if (homePanel_.has_value())
        {
            const auto it = std::find(entries_.begin(), entries_.end(), *homePanel_);
            if (it != entries_.end())
            {
                currentIndex_ = static_cast<std::size_t>(std::distance(entries_.begin(), it));
                return &entries_[currentIndex_];
            }
        }

        currentIndex_ = 0;
        return &entries_.front();
    }

    const String* PanelHistory::Current() const noexcept
    {
        if (entries_.empty())
            return nullptr;

        return &entries_[currentIndex_];
    }

    void PanelHistory::Clear()
    {
        entries_.clear();
        currentIndex_ = 0;
        homePanel_.reset();
    }

    void PanelHistory::SetHome(const String& panelName)
    {
        if (panelName.empty())
        {
            homePanel_.reset();
            return;
        }

        homePanel_ = panelName;
    }
}
