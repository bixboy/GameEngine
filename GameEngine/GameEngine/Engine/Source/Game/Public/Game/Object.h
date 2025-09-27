#pragma once

#include <string>
#include <string_view>
#include <istream>   // 👈 nécessaire pour std::istream::read
#include <ostream>   // 👈 nécessaire pour std::ostream::write

#include "Math/Math.h"

namespace Engine::Game
{
    class Object
    {
    public:
        Object();
        explicit Object(std::string name);
        Object(std::string name, const Math::Transform& transform);
        virtual ~Object() = default;

        // ... (le reste de ton code identique)

        virtual void SerializeBinary(std::ostream& stream) const;
        virtual void DeserializeBinary(std::istream& stream);

    protected:
        virtual void SerializeBinaryImpl(std::ostream& stream) const {}
        virtual void DeserializeBinaryImpl(std::istream& stream) {}

        template<typename T>
        static void WritePrimitive(std::ostream& stream, const T& value)
        {
            stream.write(reinterpret_cast<const char*>(&value), sizeof(T));
        }

        template<typename T>
        static void ReadPrimitive(std::istream& stream, T& value)
        {
            stream.read(reinterpret_cast<char*>(&value), sizeof(T));
        }

        static void WriteString(std::ostream& stream, std::string_view value);
        static std::string ReadString(std::istream& stream);

    private:
        static std::string GenerateUUID();

        void SerializeTransform(std::ostream& stream) const;
        void DeserializeTransform(std::istream& stream);

        std::string uuid_;
        std::string name_;
        Math::Transform transform_{};
    };
}