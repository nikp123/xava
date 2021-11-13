#ifndef __XAVA_SHARED_VERSION_H
#define __XAVA_SHARED_VERSION_H
#include <stdbool.h>

extern enum XAVA_VERSION_COMPATIBILITY {
    XAVA_VERSIONS_COMPATIBLE,
    XAVA_VERSIONS_INCOMPATIBLE,
    XAVA_VERSIONS_UNKNOWN,
} XAVA_VERSION_COMPATIBILITY;

extern struct xava_version {
    int major;
    int minor;
    int tweak;
    int patch;
} xava_version;

struct xava_version xavaGetHostVersion(void);
bool xavaIsVersionLess(struct xava_version host, struct xava_version target);
bool xavaIsVersionGreater(struct xava_version host, struct xava_version target);
bool xavaIsVersionEqual(struct xava_version host, struct xava_version target);
bool xavaIsBreakingVersion(struct xava_version target);
enum XAVA_VERSION_COMPATIBILITY xavaVerifyVersion(struct xava_version target);
#endif

