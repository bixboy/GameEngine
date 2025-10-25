#pragma once

/**
 * @brief Marks a class as participating in the reflection system.
 */
#define BCLASS(...)

/**
 * @brief Marks a field to be reflected by the tooling.
 */
#define BPROPERTY(...)

#ifndef GENERATED_BODY
/**
 * @brief Ensures generated code is included for reflected types.
 */
#    define GENERATED_BODY(...) static_assert(false, "Missing generated header for this class. Did you run BixHeaderTool?")
#endif
