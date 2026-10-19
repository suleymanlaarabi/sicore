#pragma once

#if defined(sicore_STATIC)
#    define SICORE_API
#elif defined(_WIN32)
#    if defined(sicore_EXPORTS)
#        define SICORE_API __declspec(dllexport)
#    else
#        define SICORE_API __declspec(dllimport)
#    endif
#elif defined(sicore_EXPORTS)
#    define SICORE_API __attribute__((visibility("default")))
#else
#    define SICORE_API
#endif
