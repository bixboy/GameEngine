#include "Bix/Reflection/PropertyInfo.h"

namespace Bix::Reflection
{
    bool PropertyInfo::IsValid() const noexcept
    {
        return static_cast<bool>(Access) && static_cast<bool>(ConstAccess);
    }

    void* PropertyInfo::GetRaw(void* instance) const
    {
        return Access ? Access(instance) : nullptr;
    }

    const void* PropertyInfo::GetRaw(const void* instance) const
    {
        return ConstAccess ? ConstAccess(instance) : nullptr;
    }
}
