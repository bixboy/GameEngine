#pragma once
#include "Ressources/Core/IResource.h"
#include <vector>
#include <cstdint>


namespace BixEngine::Resources
{
    class AudioClip : public IResource
    {
    public:
        AudioClip() = default;
        ~AudioClip() override = default;

        AudioClip(const AudioClip&) = delete;
        AudioClip& operator=(const AudioClip&) = delete;

        AudioClip(AudioClip&&) noexcept = default;
        AudioClip& operator=(AudioClip&&) noexcept = default;

        // --- IResource ---
        bool LoadFromFile(const String& path) override;
        
        // --- Getters ---
        [[nodiscard]] const std::vector<uint8_t>& GetData() const noexcept { return data_; }
        [[nodiscard]] size_t GetSize() const noexcept { return data_.size(); }
        [[nodiscard]] const String& GetPath() const noexcept { return path_; }

    private:
        std::vector<uint8_t> data_;
        String path_;
    };
}