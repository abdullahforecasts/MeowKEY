
#ifndef PLATFORM_DETECTOR_HPP
#define PLATFORM_DETECTOR_HPP

// Platform detection macros
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
#elif defined(__linux__)
    #define PLATFORM_LINUX
#else
    #error "Unsupported platform"
#endif

#endif