#include "Engine/Gui/Dialogs/ModalDialog.h"
#include <imgui.h>

namespace BixEngine::Gui
{

    void ModalDialog::Render()
    {
        if (!bIsOpen_ && !openRequested_)
            return;

        if (openRequested_)
        {
            ImGui::OpenPopup(popupId_);
            openRequested_ = false;
        }

        bool open = true;
        if (ImGui::BeginPopupModal(popupId_, &open, ImGuiWindowFlags_AlwaysAutoResize))
        {
            DrawContent();
            ImGui::EndPopup();
        }

        if (!open || !ImGui::IsPopupOpen(popupId_))
        {
            bIsOpen_ = false;
        }
    }

    void ModalDialog::Open()
    {
        openRequested_ = true;
        bIsOpen_ = true;
    }

    void ModalDialog::Close()
    {
        ImGui::CloseCurrentPopup();
        bIsOpen_ = false;
    }

    bool ModalDialog::IsOpen() const
    {
        return bIsOpen_;
    }
}
