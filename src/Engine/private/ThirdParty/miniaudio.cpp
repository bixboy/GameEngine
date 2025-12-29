




#define MA_NO_SSE2
#define MA_NO_AVX2
#define MA_NO_AVX512


#pragma warning(push)
#pragma warning(disable: 4456) 
#pragma warning(disable: 4245) 
#pragma warning(disable: 4389) 
#pragma warning(disable: 4244) 
#pragma warning(disable: 4267) 

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#pragma warning(pop)
