#pragma once

#define BCLASS(...)

#define BPROPERTY(...)

#ifndef GENERATED_BODY
#    define GENERATED_BODY(...) static_assert(false, "Missing generated header for this class. Did you run BixHeaderTool?")
#endif
