#pragma once
#include <filesystem>
#include "Containers/String.h"
#include "Gui/Panels/ContentBrowser/ContentBrowserState.h"

namespace BixEngine::Gui
{
    using BixEngine::String;
    using std::filesystem::path;
    namespace fs = std::filesystem;

    class ModalDialog
    {
    public:
        ModalDialog(ContentBrowserState& state, String& selectedEntry, const char* popupId) :
            state_(state),
            selectedEntry_(selectedEntry),
            popupId_(popupId),
            openRequested_(false),
            bIsOpen_(false)
        {
        }

        virtual ~ModalDialog() = default;

        void Render();

        virtual void Open();
        virtual void Close();

        bool IsOpen() const;

    protected:
        virtual void DrawContent() = 0;

        ContentBrowserState& state_;
        String& selectedEntry_;

    private:
        const char* popupId_;
        bool openRequested_;
        bool bIsOpen_;
    };
}
