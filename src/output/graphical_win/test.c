#include <stdbool.h>
#include <windows.h>

bool am_i_win32(void) {
    HMODULE module = GetModuleHandle(NULL);
    if(module == NULL)
        return false;

    // don't close the handle as it would mess up windows
    return true;
}
