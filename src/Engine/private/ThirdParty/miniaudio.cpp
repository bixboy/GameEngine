// ╔══════════════════════════════════════════════════════════════╗
// ║ miniaudio Implementation Compilation Unit                    ║
// ╚══════════════════════════════════════════════════════════════╝

// Disable SIMD to avoid alignment issues for now (debugging heap corruption)
#define MA_NO_SSE2
#define MA_NO_AVX2
#define MA_NO_AVX512

// Disable miniaudio warnings
#pragma warning(push)
#pragma warning(disable: 4456) // declaration hides previous local declaration
#pragma warning(disable: 4245) // signed/unsigned conversion
#pragma warning(disable: 4389) // signed/unsigned mismatch
#pragma warning(disable: 4244) // conversion from 'type1' to 'type2'
#pragma warning(disable: 4267) // conversion from 'size_t' to 'type'

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#pragma warning(pop)
