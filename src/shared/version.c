#include "../shared.h"

EXP_FUNC struct xava_version xavaGetHostVersion(void) {
    struct xava_version v;
    v.major = XAVA_VERSION_MAJOR;
    v.minor = XAVA_VERSION_MINOR;
    v.tweak = XAVA_VERSION_TWEAK;
    v.patch = XAVA_VERSION_PATCH;
    return v;
}

EXP_FUNC bool xavaIsVersionLess(struct xava_version host, struct xava_version target)
{
    if(target.major > host.major) {
        return false;
    } else if (target.major < host.major) {
        return true;
    }

    if(target.minor > host.minor) {
        return false;
    } else if (target.minor < host.minor) {
        return true;
    }

    if(target.tweak > host.tweak) {
        return false;
    } else if (target.tweak < host.tweak) {
        return true;
    }

    if(target.patch > host.patch) {
        return false;
    } else if (target.patch < host.patch) {
        return true;
    }

    return false;
}

EXP_FUNC bool xavaIsVersionGreater(struct xava_version host, struct xava_version target)
{
    if(target.major > host.major) {
        return true;
    } else if (target.major < host.major) {
        return false;
    }

    if(target.minor > host.minor) {
        return true;
    } else if (target.minor < host.minor) {
        return false;
    }

    if(target.tweak > host.tweak) {
        return true;
    } else if (target.tweak < host.tweak) {
        return false;
    }

    if(target.patch > host.patch) {
        return true;
    } else if (target.patch < host.patch) {
        return false;
    }

    return false;
}

EXP_FUNC bool xavaIsVersionEqual(struct xava_version host, struct xava_version target)
{
    if(target.major == host.major && target.minor == host.minor &&
        target.tweak == host.tweak && target.patch == host.patch) {
        return true;
    }
    return false;
}

EXP_FUNC bool xavaIsBreakingVersion(struct xava_version target)
{
    // list of all versions that broke something in terms of internal variables
    struct xava_version breaking_versions[] = {
        {0, 7, 1, 1},
    };

    for(int i = 0; i < sizeof(breaking_versions)/sizeof(struct xava_version); i++) {
        if(xavaIsVersionLess(breaking_versions[i], target)) {
            return true;
        }
    }

    return false;
}

EXP_FUNC enum XAVA_VERSION_COMPATIBILITY xavaVerifyVersion(struct xava_version target)
{
    struct xava_version host = xavaGetHostVersion();

    if(xavaIsVersionEqual(host, target)) {
        return XAVA_VERSIONS_COMPATIBLE;
    }

    if(xavaIsBreakingVersion(target)) {
        xavaError("The version of the module is NOT compatible with the current build");
        return XAVA_VERSIONS_INCOMPATIBLE;
    }

    if(xavaIsVersionGreater(host, target)) {
        xavaError("Please update XAVA to run this.");
        return XAVA_VERSIONS_INCOMPATIBLE;
    }

    xavaWarn("Versions may not be compatbile");
    return XAVA_VERSIONS_UNKNOWN;
}

