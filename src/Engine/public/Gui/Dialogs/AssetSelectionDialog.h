#pragma once
#include "Gui/Dialogs/ModalDialog.h"
#include <vector>
#include <string>
#include <functional>

namespace BixEngine::Gui
{
    class AssetSelectionDialog
    {
    public:
        AssetSelectionDialog();
        ~AssetSelectionDialog() = default;

        // Returns true if a selection was made.
        // outSelectedPath will contain the selected path.
        bool Render(const std::string& popupId, const std::vector<std::string>& allowedExtensions, std::string& outSelectedPath);

        void Open();
        void Close();

    private:
        void DrawGrid(const std::vector<std::string>& extensions);
        void RefreshCache(const std::vector<std::string>& extensions);

        bool isOpen_ = false;
        bool openRequested_ = false;
        bool selectionConfirmed_ = false;
        
        std::string selectedPath_;
        char searchBuffer_[128]{};
        
        // Cache
        struct AssetEntry
        {
            std::string name;
            std::string path;
            std::string extension;
        };
        std::vector<AssetEntry> cachedAssets_;
        bool cacheDirty_ = true;
    };
}
