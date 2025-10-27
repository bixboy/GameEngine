#include "Game/SceneManager.h"
#include "Core/Logger.h"

namespace BixEngine::Game
{
    SceneManager::~SceneManager()
    {
        if (scene_)
        {
            scene_->OnExit();
            scene_.reset();
        }
        activeScene_ = nullptr;
        ClearHistory();
    }

    void SceneManager::SetScene(std::unique_ptr<Scene> newScene) noexcept
    {
        ActivateScene(std::move(newScene));
    }

    void SceneManager::SetContext(SceneContext context) noexcept
    {
        context_ = context;

        if (scene_)
            scene_->SetContext(context_);
    }

    void SceneManager::ActivateScene(std::unique_ptr<Scene> newScene) noexcept
    {
        if (scene_)
        {
            scene_->OnExit();
            LOG_INFO("Scene exited: " + scene_->GetName());
        }

        scene_ = std::move(newScene);

        if (scene_)
        {
            scene_->SetContext(context_);
            scene_->OnInitialize();
            
            activeScene_ = scene_.get();
            scene_->OnEnter();

            LOG_INFO("Scene activated: " + scene_->GetName());
        }
        else
        {
            activeScene_ = nullptr;
        }
    }

    void SceneManager::ReloadScene()
    {
        if (!scene_)
            return;

        LOG_INFO("Reloading scene: " + scene_->GetName());

        const std::type_index typeIndex{typeid(*scene_)};

        auto oldScene = std::move(scene_);
        oldScene->OnExit();

        std::unique_ptr<Scene> newScene;

        if (typeIndex == std::type_index(typeid(*oldScene)))
        {
            newScene = std::make_unique<std::remove_reference_t<decltype(*oldScene)>>();
        }

        ActivateScene(std::move(newScene));
    }

    void SceneManager::PushScene(std::unique_ptr<Scene> newScene)
    {
        if (scene_)
        {
            scene_->OnPause();
            history_.push(std::move(scene_));
        }

        ActivateScene(std::move(newScene));
    }

    void SceneManager::PopScene()
    {
        if (scene_)
        {
            scene_->OnExit();
            LOG_INFO("Scene popped: " + scene_->GetName());
        }

        if (!history_.empty())
        {
            scene_ = std::move(history_.top());
            history_.pop();

            activeScene_ = scene_.get();
            scene_->OnResume();

            LOG_INFO("Resumed previous scene: " + scene_->GetName());
        }
        else
        {
            scene_.reset();
            activeScene_ = nullptr;
            LOG_WARNING("No previous scene to return to.");
        }
    }

    void SceneManager::ClearHistory()
    {
        while (!history_.empty())
            history_.pop();
    }
}
