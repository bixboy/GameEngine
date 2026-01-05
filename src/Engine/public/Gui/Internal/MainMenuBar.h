#pragma once
#include <functional>
#include <string>

namespace BixEngine::Gui
{
    class GuiManager;
    class GuiLayoutManager;
    class EditorSceneManager;
    class GuiPanel;

    class MainMenuBar
    {
    public:
        MainMenuBar(GuiManager& guiManager, GuiLayoutManager& layoutManager, EditorSceneManager& sceneManager);

        void Draw();

        void SetMenuPanelFilter(std::function<bool(GuiPanel*)> filter) { menuPanelFilter_ = std::move(filter); }

    private:
        GuiManager& guiManager_;
        GuiLayoutManager& layoutManager_;
        EditorSceneManager& sceneManager_;

        bool showEditorPreferences_{false};
        std::function<bool(GuiPanel*)> menuPanelFilter_;

        void DrawFileMenu_();
        void DrawEditMenu_();
        void DrawWindowsMenu_();
    };
}
