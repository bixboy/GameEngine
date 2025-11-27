#include "Framework/SceneManager.h"
#include <utility>
#include "Debug/Logger.h"
#include "Framework/SceneRegistry.h"


namespace BixEngine::Game
{
    namespace
    {
        void LogSceneCreationFailure(const String& name)
        {
            LOG_ERROR("Failed to create scene: " + name);
        }
    }

    SceneManager::SceneManager()
    {
        if (activeManager_ && activeManager_ != this)
        {
            LOG_WARNING("Replacing existing active SceneManager instance.");
        }

        activeManager_ = this;
    }

    SceneManager::~SceneManager()
    {
        DestroyActiveScene();
        loadedScenes_.clear();

        if (activeManager_ == this)
            activeManager_ = nullptr;
    }

    void SceneManager::SetScene(std::unique_ptr<Scene> newScene) noexcept
    {
        if (!newScene)
        {
            DestroyActiveScene();
            return;
        }

        const String newName = newScene->GetName();
        ActivateScene(std::move(newScene), newName, false);
    }

    void SceneManager::SetContext(SceneContext context) noexcept
    {
        context_ = context;

        if (activeScene_)
            activeScene_->SetContext(context_);

        for (auto& [_, entry] : loadedScenes_)
        {
            if (entry.instance)
                entry.instance->SetContext(context_);
        }
    }

    void SceneManager::ChangeSceneByName(const String& name)
    {
        if (name.empty())
        {
            LOG_ERROR("Cannot change to a scene with an empty name.");
            return;
        }

        if (activeScene_ && activeSceneName_ == name)
        {
            LOG_INFO("Reloading active scene: " + name);
        }

        LoadedScene loaded = AcquireScene(name);
        if (!loaded.instance)
        {
            LogSceneCreationFailure(name);
            return;
        }

        ActivateScene(std::move(loaded.instance), name, loaded.initialized);
    }

    void SceneManager::LoadScene(const String& name)
    {
        if (name.empty())
        {
            LOG_ERROR("Cannot load a scene with an empty name.");
            return;
        }

        if (name == activeSceneName_)
        {
            LOG_INFO("Scene is already active, skipping preload: " + name);
            return;
        }

        if (loadedScenes_.contains(name))
        {
            LOG_INFO("Scene already loaded: " + name);
            return;
        }

        LoadedScene entry;
        entry.instance = SceneRegistry::Create(name);

        if (!entry.instance)
        {
            LogSceneCreationFailure(name);
            return;
        }

        ApplyContext(*entry.instance);

        entry.instance->OnInitialize();
        entry.initialized = true;

        loadedScenes_.emplace(name, std::move(entry));
        LOG_INFO("Scene preloaded: " + name);
    }

    void SceneManager::UnloadScene(const String& name)
    {
        if (name.empty())
        {
            LOG_ERROR("Cannot unload a scene with an empty name.");
            return;
        }

        if (name == activeSceneName_)
        {
            LOG_ERROR("Cannot unload the active scene: " + name);
            return;
        }

        const auto it = loadedScenes_.find(name);
        if (it == loadedScenes_.end())
        {
            LOG_WARNING("Attempted to unload a scene that is not loaded: " + name);
            return;
        }

        loadedScenes_.erase(it);
        LOG_INFO("Scene unloaded: " + name);
    }

    void SceneManager::ReloadScene()
    {
        if (activeSceneName_.empty())
        {
            LOG_WARNING("No active scene to reload.");
            return;
        }

        const String sceneToReload = activeSceneName_;
        DestroyActiveScene();
        ChangeSceneByName(sceneToReload);
    }

    SceneManager::LoadedScene SceneManager::AcquireScene(const String& name)
    {
        if (auto it = loadedScenes_.find(name); it != loadedScenes_.end())
        {
            LoadedScene entry = std::move(it->second);
            loadedScenes_.erase(it);
            return entry;
        }

        LoadedScene entry;
        entry.instance = SceneRegistry::Create(name);
        entry.initialized = false;
        return entry;
    }

    void SceneManager::ActivateScene(std::unique_ptr<Scene> newScene, const String& name, bool alreadyInitialized)
    {
        if (!newScene)
        {
            LOG_ERROR("Cannot activate a null scene.");
            return;
        }

        DestroyActiveScene();

        ApplyContext(*newScene);

        if (!alreadyInitialized)
        {
            newScene->OnInitialize();
            alreadyInitialized = true;
        }

        LOG_INFO("Opening scene: " + name);

        activeSceneName_ = name;
        activeSceneInitialized_ = alreadyInitialized;
        activeScene_ = std::move(newScene);

        activeScene_->OnEnter();
        LOG_INFO("Entered scene: " + activeSceneName_);
    }

    void SceneManager::DestroyActiveScene() noexcept
    {
        if (!activeScene_)
            return;

        LOG_INFO("Exiting Scene: " + activeSceneName_);
        activeScene_->OnExit();
        activeScene_.reset();
        activeSceneInitialized_ = false;
        activeSceneName_.clear();
    }

    void SceneManager::ApplyContext(Scene& scene) const noexcept
    {
        scene.SetContext(context_);
    }
}
