// Portable errno -> string helper
#pragma once

#include <string>
#include <cstring>

namespace StormByte {
namespace Network {

inline std::string errno_to_string(int errnum) {
#ifdef _MSC_VER
    char buf[256] = {0};
    if (strerror_s(buf, sizeof(buf), errnum) == 0) return std::string(buf);
    return std::to_string(errnum);
#else
    char buf[256] = {0};
    // Handle both GNU (returns char*) and POSIX (returns int) strerror_r variants
#if defined(__GLIBC__) && !defined(__APPLE__)
    // GNU-specific: may return char*
    char *msg = strerror_r(errnum, buf, sizeof(buf));
    return std::string(msg ? msg : "Unknown error");
#else
    // POSIX: returns int, 0 on success
    if (strerror_r(errnum, buf, sizeof(buf)) == 0) return std::string(buf);
    return std::to_string(errnum);
#endif
#endif
}

} // namespace Network
} // namespace StormByte
