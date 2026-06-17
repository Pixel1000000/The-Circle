#pragma once

// Enabled only when compiled with -DTC_DEBUG_LOG=ON (cmake option).
// Usage:
//   TC_LOG("CombatSystem", "ranged view=%d pending=%d", viewSize, pendingSize);
//   TC_FRAME_TICK();   // call once per frame
//
// Output goes to debug_log.txt in the working directory (next to the exe).
// The file is truncated on each launch so each run is clean.

#ifdef TC_DEBUG_LOG

#include <cstdio>
#include <fstream>

namespace tc::debug {

inline std::ofstream& logFile()
{
    static std::ofstream file("debug_log.txt", std::ios::trunc);
    return file;
}

inline int& frame()
{
    static int n = 0;
    return n;
}

} // namespace tc::debug

#define TC_FRAME_TICK() (++tc::debug::frame())

#define TC_LOG(cat, ...)                                              \
    do {                                                              \
        char _buf[512];                                               \
        std::snprintf(_buf, sizeof(_buf), __VA_ARGS__);              \
        tc::debug::logFile()                                          \
            << "[F" << tc::debug::frame()                            \
            << "][" << (cat) << "] " << _buf << "\n";               \
        tc::debug::logFile().flush();                                 \
    } while (0)

#else

#define TC_FRAME_TICK()       do {} while (0)
#define TC_LOG(cat, ...)      do {} while (0)

#endif
