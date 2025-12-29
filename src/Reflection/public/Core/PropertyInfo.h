#pragma once
#include <functional>

#include <string>
#include <unordered_map>
#include <memory>


namespace Bix::Reflection
{
    struct ClassInfo;

    struct PropertyInfo
    {
        using Getter = std::function<void*(void*)>;
        using ConstGetter = std::function<const void*(const void*)>;

        std::string Name;
        std::string TypeName;
        std::string Metadata;
        std::unordered_map<std::string, std::string> ParsedMetadata;

        std::size_t Offset = 0;
        bool HasOffset = false;

        std::size_t Size = 0;

        const ClassInfo* Owner = nullptr;

        Getter Access;
        ConstGetter ConstAccess;

        [[nodiscard]] bool IsValid() const noexcept;

        void* GetRaw(void* instance) const;

        const void* GetRaw(const void* instance) const;

        template <typename T>
        T& Get(void* instance) const { return *static_cast<T*>(GetRaw(instance)); }

        template <typename T>
        const T& Get(const void* instance) const { return *static_cast<const T*>(GetRaw(instance)); }


        bool HasMetadata(const std::string& key) const;
        std::string GetMetadata(const std::string& key) const;
        void ParseMetadata();

        struct ArrayAccess
        {
            std::function<void(void*)> Clear;
            std::function<void(void*, const std::string&)> AddString; 
            std::function<std::size_t(const void*)> GetSize;
            std::function<std::string(const void*, std::size_t)> GetStringAt;
        };
        
        std::shared_ptr<ArrayAccess> ArrayFunctions = nullptr; 
    };
}
