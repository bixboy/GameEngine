#pragma once
#include "Containers/String.h"
#include <type_traits>
#include <functional>


namespace BixEngine
{
    template<typename T>
    requires std::is_class_v<T>
    class TSubclassOf
    {
    public:
        TSubclassOf() = default;
        
        TSubclassOf(const TSubclassOf&) = default;
        TSubclassOf(TSubclassOf&&) noexcept = default;
        TSubclassOf& operator = (const TSubclassOf&) = default;
        TSubclassOf& operator = (TSubclassOf&&) noexcept = default;

        explicit TSubclassOf(String path) : path_(std::move(path)) {}
        explicit TSubclassOf(const char* path) : path_(path) {}

        // --- Accesseurs ---
        
        [[nodiscard]] const String& GetAssetPath() const { return path_; }
        [[nodiscard]] bool IsValid() const { return !path_.empty(); }
        
        explicit operator bool() const { return IsValid(); }

        // --- Conversions ---
        [[nodiscard]] String ToString() const { return path_; }

        // --- Comparaisons ---

        bool operator == (const TSubclassOf& other) const { return path_ == other.path_; }
        bool operator != (const TSubclassOf& other) const { return !(*this == other); }

        bool operator == (const String& otherPath) const { return path_ == otherPath; }
        bool operator == (const char* otherPath) const { return path_ == otherPath; }

    private:
        String path_;
    };

}


namespace std
{
    template<typename T>
    struct hash<BixEngine::TSubclassOf<T>>
    {
        std::size_t operator()(const BixEngine::TSubclassOf<T>& k) const noexcept
        {
            return std::hash<BixEngine::String>{}(k.GetAssetPath());
        }
    };
}