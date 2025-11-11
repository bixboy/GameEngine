#include "Engine/Gui/Dialogs/CreateFolderDialog.h"
#include "Engine/Gui/Dialogs/CreatePrefabDialog.h"
#include "Engine/Gui/Dialogs/CreateScriptDialog.h"
#include "Engine/Gui/Dialogs/CreateSpriteAtlasDialog.h"
#include "Engine/Gui/Dialogs/RenameEntryDialog.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserState.h"


namespace BixEngine::Gui
{

    void RenderPopups(ContentBrowserState& state, String& selectedEntry, PopupRequestState& requestPopups)
    {
        static CreatePrefabDialog prefabDialog(state, selectedEntry);
        static CreateScriptDialog scriptDialog(state, selectedEntry);
        static CreateFolderDialog folderDialog(state, selectedEntry);
        static CreateSpriteAtlasDialog spriteAtlasDialog(state, selectedEntry);
        static RenameEntryDialog renameDialog(state, selectedEntry);

        // ─────────── OUVERTURE ───────────
        if (requestPopups.createPrefab)
        {
            prefabDialog.Open();
            requestPopups.createPrefab = false;
        }
        
        if (requestPopups.createScript)
        {
            scriptDialog.Open();
            requestPopups.createScript = false;
        }
        
        if (requestPopups.createFolder)
        {
            folderDialog.Open(state.current);
            requestPopups.createFolder = false;
        }
        
        if (requestPopups.createSpriteAtlas)
        {
            spriteAtlasDialog.Open(state.current);
            requestPopups.createSpriteAtlas = false;
        }
        
        if (requestPopups.renameEntry)
        {
            renameDialog.Open(requestPopups.renameTarget, requestPopups.renameSecondaryTarget, requestPopups.renameTargetIsScriptGroup);
            requestPopups.renameEntry = false;
        }

        if (prefabDialog.IsOpen())
            prefabDialog.Render();

        if (scriptDialog.IsOpen())
            scriptDialog.Render();

        if (folderDialog.IsOpen())
            folderDialog.Render();

        if (spriteAtlasDialog.IsOpen())
            spriteAtlasDialog.Render();

        if (renameDialog.IsOpen())
            renameDialog.Render();
    }

}
