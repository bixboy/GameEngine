#pragma once
#include <iostream>
#include <cstdint>
#include <vector>
#include <type_traits>

#include "Containers/Array.h"
#include "Containers/String.h"


namespace Bix::Reflection
{
    struct ClassInfo;
}

namespace BixEngine::Utils
{
    // ======= Binary Writer =======
    class BinaryWriter
    {
    public:
        explicit BinaryWriter(std::ostream& stream);

        bool WriteUint32(std::uint32_t value);
        bool WriteString(const String& value);

        template <typename T>
        bool WritePrimitive(const T& value)
        {
            stream_->write(reinterpret_cast<const char*>(&value), sizeof(T));
            return static_cast<bool>(stream_);
        }
        
        template <typename T>
        bool WriteVector(const std::vector<T>& vec)
        {
            return WriteContainerImpl(vec);
        }
        
        template <typename T>
        bool WriteArray(const TArray<T>& arr)
        {
            return WriteContainerImpl(arr);
        }

        [[nodiscard]] bool Good() const noexcept;

    private:
        bool WriteBytes(const char* data, std::size_t length);

        template <typename Container>
        bool WriteContainerImpl(const Container& container)
        {
            using T = Container::value_type;

            if (!WriteUint32(static_cast<std::uint32_t>(container.size()))) 
                return false;

            if constexpr (std::is_trivially_copyable_v<T>)
            {
                if (!container.empty())
                    return WriteBytes(reinterpret_cast<const char*>(container.data()), container.size() * sizeof(T));
            }
            else
            {
                for (const auto& item : container)
                {
                    if (!WritePrimitive(item))
                        return false;
                }
            }
            return true;
        }
        
        std::ostream* stream_{nullptr};
    };


    // ======= Binary Reader =======
    class BinaryReader
    {
    public:
        explicit BinaryReader(std::istream& stream);

        bool ReadBytes(char* data, std::size_t length);
        bool ReadUint32(std::uint32_t& value);
        bool ReadString(String& value);

        template <typename T>
        bool ReadPrimitive(T& value)
        {
            stream_->read(reinterpret_cast<char*>(&value), sizeof(T));
            return static_cast<bool>(stream_);
        }

        template <typename T>
        bool ReadVector(std::vector<T>& vec)
        {
            return ReadContainerImpl(vec);
        }
        
        template <typename T>
        bool ReadArray(TArray<T>& arr)
        {
            return ReadContainerImpl(arr);
        }

        [[nodiscard]] bool Good() const noexcept;

    private:
        
        template <typename Container>
        bool ReadContainerImpl(Container& container)
        {
            using T = Container::value_type;

            std::uint32_t size = 0;
            if (!ReadUint32(size))
                return false;

            if (size > 10000000)
                return false;

            container.resize(size);

            if constexpr (std::is_trivially_copyable_v<T>)
            {
                if (size > 0)
                    return ReadBytes(reinterpret_cast<char*>(container.data()), size * sizeof(T));
            }
            else
            {
                for (auto& item : container)
                {
                    if (!ReadPrimitive(item))
                        return false;
                }
            }
            return true;
        }
        
        std::istream* stream_{nullptr};
    };
}