#include "Components/AudioSourceComponent.h"
#include "Components/AudioSourceComponent.h"
#include "Actor.h"
#include "Math/Transform.h"
#include <miniaudio.h>
#include "Logger.h"
#include "Ressources/ResourceManager.h"
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <imgui.h>

namespace BixEngine::Game
{
    struct AudioSourceComponent::Impl
    {
        ma_sound sound;
        ma_decoder decoder;
        bool initialized = false;
    };

    AudioSourceComponent::AudioSourceComponent(Actor* owner) : Component(owner), impl_(std::make_unique<Impl>())
    {
    }

    AudioSourceComponent::~AudioSourceComponent()
    {
        if (impl_->initialized)
        {
            ma_sound_uninit(&impl_->sound);
            ma_decoder_uninit(&impl_->decoder);
        }
    }

    void AudioSourceComponent::BeginPlay()
    {
        if (!AudioClip)
            return;

        ma_engine* engine = Systems::AudioSystem::Get().GetEngine();
        if (!engine)
        {
            LOG_ERROR("AudioSystem engine is null.");
            return;
        }

        ma_uint32 flags = 0;
        if (IsMusic)
        {
            flags |= MA_SOUND_FLAG_STREAM;
        }
        else
        {
            flags |= MA_SOUND_FLAG_DECODE;
        }
        
        if (!Is3D)
            flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;

        // Init decoder
        ma_decoder_config decoderConfig = ma_decoder_config_init_default();
        ma_result result = ma_decoder_init_memory(AudioClip->GetData().data(), AudioClip->GetData().size(), &decoderConfig, &impl_->decoder);

        if (result != MA_SUCCESS)
        {
            LOG_ERROR("Failed to init decoder from memory for AudioClip");
            return;
        }

        // Init sound from data source
        result = ma_sound_init_from_data_source(
            engine, 
            &impl_->decoder, 
            flags, 
            NULL, 
            &impl_->sound
        );

        if (result != MA_SUCCESS)
        {
            LOG_ERROR("Failed to init sound from data source");
            ma_decoder_uninit(&impl_->decoder);
            return;
        }

        impl_->initialized = true;
        ma_sound_set_volume(&impl_->sound, Volume);
        ma_sound_set_looping(&impl_->sound, Loop);
        
        // Initial position
        if (Is3D && owner_)
        {
            auto pos = owner_->GetTransform().GetPosition();
            ma_sound_set_position(&impl_->sound, pos.x, pos.y, pos.z);
        }
    }

    void AudioSourceComponent::Update(float dt)
    {
        if (!impl_->initialized)
            return;

        if (Is3D && owner_)
        {
            auto pos = owner_->GetTransform().GetPosition();
            ma_sound_set_position(&impl_->sound, pos.x, pos.y, pos.z);
        }
    }
    
    void AudioSourceComponent::Play()
    {
        if (impl_->initialized)
        {
            ma_sound_start(&impl_->sound);
        }
    }

    void AudioSourceComponent::Stop()
    {
        if (impl_->initialized)
        {
            ma_sound_stop(&impl_->sound);
        }
    }

    void AudioSourceComponent::SetVolume(float volume)
    {
        Volume = volume;
        if (impl_->initialized)
        {
            ma_sound_set_volume(&impl_->sound, Volume);
        }
    }

    namespace
    {
        std::filesystem::path DetermineAudioRoot()
        {
            std::error_code ec;
            const std::filesystem::path base = std::filesystem::current_path(ec);
            if (ec)
                return {};

            const std::filesystem::path content = base / "Content";
            if (std::filesystem::exists(content))
                return content;

            const std::filesystem::path resources = base / "Resources";
            if (std::filesystem::exists(resources))
                return resources;

            return {};
        }

        void CollectAudioFiles(std::vector<std::filesystem::path>& outFiles)
        {
            const std::filesystem::path root = DetermineAudioRoot();
            if (root.empty())
                return;

            std::error_code ec;
            for (std::filesystem::recursive_directory_iterator it(root, ec), end; it != end; it.increment(ec))
            {
                if (ec)
                    break;

                if (!it->is_regular_file())
                    continue;

                std::string extension = it->path().extension().generic_string();
                std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c){ return std::tolower(c); });
                
                if (extension == ".mp3" || extension == ".wav" || extension == ".ogg")
                    outFiles.push_back(it->path());
            }

            std::sort(outFiles.begin(), outFiles.end(),
              [](const std::filesystem::path& a, const std::filesystem::path& b)
              {
                  return a.generic_string() < b.generic_string();
              });
        }

        std::string MakeDisplayName(const std::filesystem::path& path, const std::filesystem::path& root)
        {
            if (root.empty())
                return path.generic_string();

            std::error_code ec;
            const std::filesystem::path relative = std::filesystem::relative(path, root, ec);
            if (!ec && !relative.empty())
                return relative.generic_string();

            return path.generic_string();
        }
    }

    void AudioSourceComponent::DrawInspectorUI()
    {
        Component::DrawInspectorUI();

        std::vector<std::filesystem::path> audioFiles;
        CollectAudioFiles(audioFiles);
        const std::filesystem::path root = DetermineAudioRoot();

        std::vector<std::string> audioPaths;
        std::vector<std::string> audioLabels;
        audioPaths.reserve(audioFiles.size());
        audioLabels.reserve(audioFiles.size());

        int currentAudioIndex = -1;
        
        // Determine current selection based on AudioClip path if available
        std::string currentPath;
        if (AudioClip)
        {
            currentPath = AudioClip->GetPath().ToStdString();
        }

        for (size_t index = 0; index < audioFiles.size(); ++index)
        {
            const std::string pathString = audioFiles[index].generic_string();
            audioPaths.push_back(pathString);
            audioLabels.push_back(MakeDisplayName(audioFiles[index], root));

            // Simple check: if current path ends with this file path (handling relative vs absolute)
            // Or exact match if we store full path
            if (!currentPath.empty())
            {
                // Normalize separators for comparison
                std::string normPath = pathString;
                std::string normCurrent = currentPath;
                std::replace(normPath.begin(), normPath.end(), '\\', '/');
                std::replace(normCurrent.begin(), normCurrent.end(), '\\', '/');
                
                if (normPath == normCurrent || normCurrent.ends_with(normPath))
                {
                    currentAudioIndex = static_cast<int>(index);
                }
            }
        }

        const char* currentAudioLabel = currentAudioIndex >= 0 ? audioLabels[currentAudioIndex].c_str() : "<None>";

        if (ImGui::BeginCombo("Audio Clip", currentAudioLabel))
        {
            const bool selectedNone = (currentAudioIndex < 0);
            if (ImGui::Selectable("<None>", selectedNone))
            {
                AudioClip.reset();
            }

            for (int i = 0; i < static_cast<int>(audioPaths.size()); ++i)
            {
                const bool selected = (i == currentAudioIndex);
                if (ImGui::Selectable(audioLabels[i].c_str(), selected))
                {
                    auto& rm = resources::ResourceManager::Get();
                    AudioClip = rm.Get<resources::AudioClip>(audioPaths[i]);
                }
            }

            ImGui::EndCombo();
        }
    }
}
