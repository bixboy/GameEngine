#pragma once

#include <cstddef>
#include <cstdint>
#include <istream>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "SDL3/SDL.h"

#include "Bix/Core/String.h"
#include "Bix/Engine/SaveSystem/BixGuid.h"
#include "Bix/Engine/SaveSystem/BixTypeTraits.h"
#include "Bix/Math/Math.h"

namespace BixEngine::Engine::SaveSystem
{
    class BixObject;
    class BixArchiveWriter
    {
    public:
        explicit BixArchiveWriter(std::ostream& stream) : stream_(stream) {}

        template<typename T>
        void WritePrimitive(const T& value)
        {
            static_assert(std::is_trivially_copyable_v<T>, "Only trivially copyable primitives can be written directly.");
            stream_.write(reinterpret_cast<const char*>(&value), sizeof(T));
        }

        void WriteGuid(const BixGuid& guid);
        void WriteString(const String& value);
        void WriteStdString(const std::string& value);
        void WriteVector2(const Math::Vector2& value);
        void WriteVector3(const Math::Vector3& value);
        void WriteRotator(const Math::Rotator& value);
        void WriteTransform(const Math::Transform& value);
        void WriteColor(const SDL_Color& value);

        void WriteObject(const BixObject* object);

        [[nodiscard]] bool Good() const noexcept { return static_cast<bool>(stream_); }

        std::ostream& GetStream() noexcept { return stream_; }

    private:
        std::ostream& stream_;
    };

    class BixArchiveReader
    {
    public:
        explicit BixArchiveReader(std::istream& stream) : stream_(stream) {}

        template<typename T>
        void ReadPrimitive(T& value)
        {
            static_assert(std::is_trivially_copyable_v<T>, "Only trivially copyable primitives can be read directly.");
            stream_.read(reinterpret_cast<char*>(&value), sizeof(T));
            if (!stream_)
                throw std::runtime_error("Failed to read primitive from archive.");
        }

        BixGuid ReadGuid();
        String ReadString();
        std::string ReadStdString();
        Math::Vector2 ReadVector2();
        Math::Vector3 ReadVector3();
        Math::Rotator ReadRotator();
        Math::Transform ReadTransform();
        SDL_Color ReadColor();

        std::unique_ptr<BixObject> ReadObject(BixObject* outer);

        [[nodiscard]] bool Good() const noexcept { return static_cast<bool>(stream_); }

        std::istream& GetStream() noexcept { return stream_; }

    private:
        std::istream& stream_;
    };

    void SerializeObject(BixArchiveWriter& writer, const BixObject& object);
    std::unique_ptr<BixObject> DeserializeObject(BixArchiveReader& reader, BixObject* outer);

    template<typename T>
    struct PropertyAdapter
    {
        static void Serialize(const BixObject&, const T& value, BixArchiveWriter& writer)
        {
            if constexpr (std::is_arithmetic_v<T>)
            {
                writer.WritePrimitive(value);
            }
            else if constexpr (std::is_same_v<T, String>)
            {
                writer.WriteString(value);
            }
            else if constexpr (std::is_same_v<T, std::string>)
            {
                writer.WriteStdString(value);
            }
            else if constexpr (std::is_same_v<T, BixGuid>)
            {
                writer.WriteGuid(value);
            }
            else if constexpr (std::is_same_v<T, Math::Vector2>)
            {
                writer.WriteVector2(value);
            }
            else if constexpr (std::is_same_v<T, Math::Vector3>)
            {
                writer.WriteVector3(value);
            }
            else if constexpr (std::is_same_v<T, Math::Rotator>)
            {
                writer.WriteRotator(value);
            }
            else if constexpr (std::is_same_v<T, Math::Transform>)
            {
                writer.WriteTransform(value);
            }
            else if constexpr (std::is_same_v<T, SDL_Color>)
            {
                writer.WriteColor(value);
            }
            else
            {
                static_assert(AlwaysFalse<T>::value, "Unsupported property type for serialization.");
            }
        }

        static void Deserialize(BixObject&, T& value, BixArchiveReader& reader)
        {
            if constexpr (std::is_arithmetic_v<T>)
            {
                reader.ReadPrimitive(value);
            }
            else if constexpr (std::is_same_v<T, String>)
            {
                value = reader.ReadString();
            }
            else if constexpr (std::is_same_v<T, std::string>)
            {
                value = reader.ReadStdString();
            }
            else if constexpr (std::is_same_v<T, BixGuid>)
            {
                value = reader.ReadGuid();
            }
            else if constexpr (std::is_same_v<T, Math::Vector2>)
            {
                value = reader.ReadVector2();
            }
            else if constexpr (std::is_same_v<T, Math::Vector3>)
            {
                value = reader.ReadVector3();
            }
            else if constexpr (std::is_same_v<T, Math::Rotator>)
            {
                value = reader.ReadRotator();
            }
            else if constexpr (std::is_same_v<T, Math::Transform>)
            {
                value = reader.ReadTransform();
            }
            else if constexpr (std::is_same_v<T, SDL_Color>)
            {
                value = reader.ReadColor();
            }
            else
            {
                static_assert(AlwaysFalse<T>::value, "Unsupported property type for deserialization.");
            }
        }
    };

    template<typename T>
    struct PropertyAdapter<std::vector<T>>
    {
        static void Serialize(const BixObject& owner, const std::vector<T>& values, BixArchiveWriter& writer)
        {
            const auto count = static_cast<std::uint32_t>(values.size());
            writer.WritePrimitive(count);
            for (const auto& value : values)
            {
                PropertyAdapter<T>::Serialize(owner, value, writer);
            }
        }

        static void Deserialize(BixObject& owner, std::vector<T>& values, BixArchiveReader& reader)
        {
            std::uint32_t count = 0;
            reader.ReadPrimitive(count);
            values.clear();
            values.reserve(count);
            for (std::uint32_t i = 0; i < count; ++i)
            {
                T element{};
                PropertyAdapter<T>::Deserialize(owner, element, reader);
                values.push_back(std::move(element));
            }
        }
    };

    template<typename T>
    struct PropertyAdapter<std::unique_ptr<T>>;

    template<typename T>
    struct PropertyAdapter<std::vector<std::unique_ptr<T>>>;
}

