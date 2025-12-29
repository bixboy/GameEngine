#pragma once
#include "Containers/String.h"

template<typename T>
class TSubclassOf
{
public:
    TSubclassOf() = default;
    TSubclassOf(const BixEngine::String& path) : path_(path) {}
    TSubclassOf(const char* path) : path_(path) {}

    const BixEngine::String& GetAssetPath() const { return path_; }
    BixEngine::String& GetAssetPath() { return path_; }
    
    bool IsValid() const { return !path_.IsEmpty(); }

    operator BixEngine::String() const { return path_; }

    bool operator==(const TSubclassOf& other) const { return path_ == other.path_; }
    bool operator!=(const TSubclassOf& other) const { return path_ != other.path_; }

private:
    BixEngine::String path_;
};
