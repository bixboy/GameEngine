#pragma once
#include "Utils/BinaryUtils.h"

namespace BixEngine::Serialization
{
    class ReflectedSerializer
    {
    public:
        static bool Serialize(const void* instance, const Bix::Reflection::ClassInfo* info, Utils::BinaryWriter& writer, int depth = 0);
        static bool Deserialize(void* instance, const Bix::Reflection::ClassInfo* info, Utils::BinaryReader& reader, int depth = 0);
    };   
}
