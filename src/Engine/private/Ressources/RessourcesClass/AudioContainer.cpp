#include "Ressources/RessourcesClass/AudioContainer.h"
#include "Ressources/Core/ResourceManager.h"
#include "Debug/Logger.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <random>


namespace BixEngine::Resources
{
    using json = nlohmann::json;

    static float RandomFloat(float min, float max)
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution dis(min, max);
        return dis(gen);
    }

    bool AudioContainer::LoadFromFile(const String& path)
    {
        path_ = path;
        std::ifstream file(path.c_str());
        if (!file.is_open())
        {
            LOG_ERROR("Failed to open AudioContainer file: " + path);
            return false;
        }

        try
        {
            json j;
            file >> j;

            VolumeVariance = j.value("VolumeVariance", 0.0f);
            PitchVariance = j.value("PitchVariance", 0.0f);
            Loop = j.value("Loop", false);
            Mode = static_cast<AudioContainerMode>(j.value("Mode", 0));

            if (j.contains("Tracks"))
            {
                Tracks.clear();
                for (const auto& trackJson : j["Tracks"])
                {
                    AudioTrack track;
                    
                    std::string stdPath = trackJson.value("ClipPath", "");
                    String clipPath = stdPath.c_str();

                    if (!clipPath.empty())
                    {
                        track.Clip = ResourceManager::Get().Get<AudioClip>(clipPath);
                    }

                    track.Weight = trackJson.value("Weight", 1.0f);
                    track.VolumeMultiplier = trackJson.value("VolumeMultiplier", 1.0f);
                    track.PitchMultiplier = trackJson.value("PitchMultiplier", 1.0f);
                    
                    Tracks.push_back(track);
                }
            }
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to parse AudioContainer JSON: " + String(e.what()));
            return false;
        }

        return true;
    }

    bool AudioContainer::SaveToFile(const String& path)
    {
        json j;
        j["VolumeVariance"] = VolumeVariance;
        j["PitchVariance"] = PitchVariance;
        j["Loop"] = Loop;
        j["Mode"] = Mode;

        json tracksJson = json::array();
        for (const auto& track : Tracks)
        {
            json trackJ;
            if (track.Clip)
            {
                trackJ["ClipPath"] = track.Clip->GetPath().c_str();
            }
            else
            {
                trackJ["ClipPath"] = "";
            }
            trackJ["Weight"] = track.Weight;
            trackJ["VolumeMultiplier"] = track.VolumeMultiplier;
            trackJ["PitchMultiplier"] = track.PitchMultiplier;
            tracksJson.push_back(trackJ);
        }
        j["Tracks"] = tracksJson;

        std::ofstream file(path.c_str());
        if (!file.is_open())
        {
            LOG_ERROR("Failed to write AudioContainer file: " + path);
            return false;
        }

        file << j.dump(4);
        return true;
    }

    ResolvedSound AudioContainer::ResolveSound()
    {
        ResolvedSound result;
        if (Tracks.empty())
            return result;

        int selectedIndex = 0;
        
        if (Mode == AudioContainerMode::Sequence)
        {
            selectedIndex = (lastPlayedIndex_ + 1) % static_cast<int>(Tracks.size());
        }
        else
        {
            float totalWeight = 0.0f;
            for (const auto& track : Tracks)
                totalWeight += track.Weight;

            if (totalWeight <= 0.0f)
            {
                 selectedIndex = 0;
            }
            else
            {
                float randomValue = RandomFloat(0.0f, totalWeight);
                float currentWeight = 0.0f;
                
                for (size_t i = 0; i < Tracks.size(); ++i)
                {
                    currentWeight += Tracks[i].Weight;
                    if (randomValue <= currentWeight)
                    {
                        selectedIndex = static_cast<int>(i);
                        break;
                    }
                }
            }
        }

        lastPlayedIndex_ = selectedIndex;
        const auto& selectedTrack = Tracks[selectedIndex];
        
        result.Clip = selectedTrack.Clip;

        result.Volume = selectedTrack.VolumeMultiplier;
        result.Pitch = selectedTrack.PitchMultiplier;

        if (VolumeVariance > 0.0f)
            result.Volume += RandomFloat(-VolumeVariance, VolumeVariance);
        
        if (PitchVariance > 0.0f)
            result.Pitch += RandomFloat(-PitchVariance, PitchVariance);

        result.Volume = std::max(0.0f, result.Volume);
        result.Pitch = std::max(0.1f, result.Pitch);

        return result;
    }
}