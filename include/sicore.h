#ifndef SICORE_H
#define SICORE_H

#define SICORE_LIKELY(x) __builtin_expect(!!(x), 1)
#define SICORE_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define SICORE_HOT __attribute__((hot))

#if defined(SICORE_VEC) && defined(SICORE_NO_VEC)
#error "SICORE_VEC and SICORE_NO_VEC cannot be defined together"
#endif

#if defined(SICORE_MAP) && defined(SICORE_NO_MAP)
#error "SICORE_MAP and SICORE_NO_MAP cannot be defined together"
#endif

#ifdef SIECS_CUSTOM_BUILD

#ifdef SICORE_VEC
#define SICORE_HAS_VEC 1
#else
#define SICORE_HAS_VEC 0
#endif

#ifdef SICORE_MAP
#define SICORE_HAS_MAP 1
#else
#define SICORE_HAS_MAP 0
#endif

#else

#ifdef SICORE_NO_VEC
#define SICORE_HAS_VEC 0
#else
#define SICORE_HAS_VEC 1
#endif

#ifdef SICORE_NO_MAP
#define SICORE_HAS_MAP 0
#else
#define SICORE_HAS_MAP 1
#endif

#endif

/* This generated file contains includes for project dependencies */
#include "sicore/bake_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif
