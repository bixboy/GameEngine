# Prefab & Asset Editor Workflow

This engine build introduces a unified asset editor system and prefab-aware Content Browser. This document highlights the core interactions that changed and the expected workflow for scripts and prefab assets.

## Creating prefab assets

1. Open the **Content Browser** and navigate to the target folder under `Content/`.
2. Use **Create > Create prefab...** (available from the background context menu or the toolbar button).
3. Pick an eligible script in the popup:
   * All user scripts parsed from `Content/Scripts/` that derive from `Actor`, `Component`, or use the `BCLASS()` macro are listed.
   * Engine-provided base classes such as `BixEngine::Game::Actor` and `BixEngine::Game::Component` are also available for quick bootstrapping.
4. Provide a name for the prefab file (a `.bixactor` is generated for actors, `.bixcomponent` for components). The engine writes a small JSON descriptor that captures:
   * The prefab type (`Actor` or `Component`).
   * The fully-qualified script class.
   * An optional include path, relative to either `Scripts/` or the project root, to help the editor locate source headers.
5. The newly created asset appears immediately in the browser and can be opened like any other asset.

The popup sanitises file names, validates JSON metadata, and prevents overwriting existing files. Include paths are kept relative whenever possible so projects remain portable across systems.

## Opening assets

* **Scripts**: double-clicking a `.h`/`.cpp` pair now opens both files in the configured external editor. The legacy Actor Editor is no longer triggered for script entries.
* **Prefabs**: double-clicking `.bixactor` or `.bixcomponent` assets opens the corresponding editor tab (Actor or Component) within the GUI. The navigation bar supports multiple prefab editors simultaneously.

## Asset editors

All asset editors derive from `BaseAssetEditorController`, which standardises:

* Toolbar layout (Play / Save / Compile actions).
* Stable ImGui identifiers and docking preferences.
* Shared metadata (asset display name, script class, include path, etc.).

`ActorEditorController` and `ComponentEditorController` extend the base behaviour with dedicated panels. The new `GuiAssetEditorManager` is responsible for:

* Spawning the right controller based on asset extension via a small factory.
* Managing editor tabs in the navigation bar and syncing visibility with the current layout.
* Restoring the Scene layout when no asset editors remain open.

The architecture intentionally keeps prefab editors independent from the active Scene so assets can be authored offline.

## File type expectations

The Content Browser and asset manager recognise the following prefab extensions:

* `.bixactor` for Actor prefabs.
* `.bixcomponent` for Component prefabs.

Legacy `.actor` / `.component` formats are no longer auto-detected. Convert existing prefabs to the new `.bix*` extensions to benefit from the unified editors.

## Future extensions

The factory-based setup inside `GuiAssetEditorManager` and `BaseAssetEditorController` is designed to accommodate new asset types (`.bixmaterial`, `.bixui`, etc.). Implementations only need to provide:

1. A controller subclass describing the panel layout and interactions.
2. Shared-state creation logic for metadata.
3. An entry in the manager's factory method to map file extensions to controller construction.

With these building blocks in place, prefab authoring and asset editing can scale alongside the engine's feature set.
