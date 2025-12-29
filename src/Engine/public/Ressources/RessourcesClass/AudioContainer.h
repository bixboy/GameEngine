#pragma once
#include "Ressources/Core/IResource.h"
#include "Ressources/RessourcesClass/AudioClip.h"
#include <vector>
#include <memory>

namespace BixEngine::resources
{
    struct AudioTrack
    {
        std::shared_ptr<AudioClip> Clip;
        float Weight = 1.0f;
        float VolumeMultiplier = 1.0f;
        float PitchMultiplier = 1.0f;
    };

    struct ResolvedSound
    {
        std::shared_ptr<AudioClip> Clip;
        float Volume = 1.0f;
        float Pitch = 1.0f;
    };

    enum class AudioContainerMode
    {
        Random,
        Sequence
    };

    class AudioContainer : public IResource
    {
    public:
        bool LoadFromFile(const String& path) override;
        bool SaveToFile(const String& path);

        ResolvedSound ResolveSound();

        
        std::vector<AudioTrack> Tracks;
        float VolumeVariance = 0.0f; 
        float PitchVariance = 0.0f;  
        bool Loop = false;
        AudioContainerMode Mode = AudioContainerMode::Random;

        const String& GetPath() const { return path_; }

    private:
        int lastPlayedIndex_ = -1;
        String path_;
    };
}
