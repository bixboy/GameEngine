#pragma once
#include "Utils/FileIO/BinaryUtils.h"

namespace Bix::Reflection { struct ClassInfo; }


namespace BixEngine::Serialization
{
    class ReflectedSerializer
    {
    public:
        // Sauvegarde un objet par réflexion
        static bool Serialize(const void* instance, const Bix::Reflection::ClassInfo* info, Utils::BinaryWriter& writer, int depth = 0);
        
        // Charge un objet par réflexion
        static bool Deserialize(void* instance, const Bix::Reflection::ClassInfo* info, Utils::BinaryReader& reader, int depth = 0);
    };   
}