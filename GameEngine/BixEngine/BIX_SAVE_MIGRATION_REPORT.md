# Bix Save System Migration Report

## Modified Files
- `Runtime/Include/Bix/Engine/SaveSystem/` — new reflection, archive, package, asset registry, and save system headers.
- `Runtime/Source/Engine/SaveSystem/` — implementations for the new save pipeline.
- `Runtime/Include/Bix/Game/Object.h` & `Runtime/Source/Game/Private/Object.cpp` — migrated base object to BixObject with reflection and GUID support.
- `Runtime/Include/Bix/Game/Actor.h` & `Runtime/Source/Game/Private/Actor.cpp` — integrated class registration, automatic component ownership, and serialization metadata.
- `Runtime/Include/Bix/Game/Components/Component.h` & related component sources — components now derive from `Object` and expose properties.
- `Runtime/Include/Bix/Game/Scene.h` & `Runtime/Source/Game/Private/Scene.cpp` — scenes participate in the new save system; legacy serializer usage removed.
- `Runtime/Include/Bix/Game/Test/Player.h` & `Runtime/Source/Game/Private/Test/Player.cpp` — player data moved to reflective properties.
- `Runtime/Include/Bix/Game/SceneSerializer.h` & `Runtime/Source/Game/Private/SceneSerializer.cpp` — downgraded to legacy wrappers around `BixSaveSystem`.
- `Runtime/Include/Bix/Game/Components/SpriteComponent.h` & `Runtime/Source/Game/Private/Components/SpriteComponent.cpp` — properties exposed for serialization and class registration added.
- `Runtime/Include/Bix/Game/EmptyScene.h` & `Runtime/Source/Game/Private/EmptyScene.cpp` — reflective metadata for editor visibility.

## Classes Migrated
- `BixEngine::Game::Object`
- `BixEngine::Game::Actor`
- `BixEngine::Game::Component` and derived `SpriteComponent`
- `BixEngine::Game::Scene` and `EmptyScene`
- `BixEngine::Game::Test::Player`

## Legacy Compatibility
- `SceneSerializer` functions remain available but are marked deprecated; they delegate to `BixSaveSystem` and rely on the reflection registry for actor creation.
- Existing binary `.scene` files are not automatically upgraded; loading through the deprecated API now reads `.bixasset` packages. Legacy JSON/DataStore pipelines have been removed.

## Outstanding Work
- Update editor tooling to call `BixSaveSystem::SavePackage` directly instead of the deprecated serializer, then remove the compatibility wrapper once adoption is complete.
- Extend `BixAssetRegistry` integration inside the editor UI so `.bixasset` metadata can be inspected in the content browser and asset pickers.
- Implement unit tests validating round-trip serialization for representative scenes, actors, and components, including cases with nested objects and empty collections.
- Audit remaining engine modules for hard-coded UUID string usage, validating error handling in `BixGuid::FromString` for malformed values.
- Plan dedicated performance and large-scene stress tests to benchmark the binary archive pipeline before shipping.

## Notes & Setup Tips
- New save system headers reside under `Runtime/Include/Bix/Engine/SaveSystem/`; ensure build scripts include this directory.
- `BIX_CLASS`/`BIX_PROPERTY` macros require corresponding `BIX_IMPLEMENT_CLASS` definitions in exactly one translation unit.
- Components created through reflection now rely on `Actor::AddComponent` or `OnPostDeserialize` hooks to establish ownership correctly.
- Deprecated `SceneSerializer` APIs remain for transition purposes and should log warnings whenever they are invoked.
