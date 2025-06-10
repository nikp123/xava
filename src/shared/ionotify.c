#include <assert.h>
#include <x-watcher.h>

#include "array.h"
#include "shared.h"
#include "shared/ionotify.h"

EXP_FUNC xava_ionotify xavaIONotifySetup(void)
{
    xava_ionotify ionotify;
    
    // WARNING: This begins a chain which leads to a very nasty reallocation bug,
    // pls rewrite this wholeass module in order to avoid it. The 100 is just a shitty
    // workaround. I do not know what causes it and I don't have the patience to debug it.
    arr_init_n(ionotify.handles, 100);

    ionotify.xwatcher_instance = xWatcher_create();

    return ionotify;
}

/**
 * This is the callback wrapper for the xWatcher library
 * The use of this is meant to be internal only.
 **/
void xavaIONotifyCallbackWrapper(XWATCHER_FILE_EVENT event, const char *path,
                                 int id, void *data)
{
    xava_ionotify_file_handle *handle = data;

    xava_ionotify_event new_event;

    switch (event)
    {
    case XWATCHER_FILE_CREATED:
    case XWATCHER_FILE_MODIFIED:
        new_event = XAVA_IONOTIFY_CHANGED;
        break;
    case XWATCHER_FILE_REMOVED:
        new_event = XAVA_IONOTIFY_DELETED;
        break;
    case XWATCHER_FILE_OPENED:             // this gets triggered by basically everything
    case XWATCHER_FILE_ATTRIBUTES_CHANGED: // usually not important
    case XWATCHER_FILE_NONE:
    case XWATCHER_FILE_RENAMED: // ignoring because most likely breaks visualizer
    case XWATCHER_FILE_UNSPECIFIED:
        new_event = XAVA_IONOTIFY_NOTHING;
        break;
    default:
        new_event = XAVA_IONOTIFY_ERROR;
    }

    handle->xava_ionotify_func(new_event, path, id, handle->xava);
}

EXP_FUNC bool xavaIONotifyAddWatch(xava_ionotify_watch_setup setup)
{
    xWatcher_reference reference;

    xava_ionotify_file_handle file_handle = {
        .xava = setup.xava, .xava_ionotify_func = setup.xava_ionotify_func};

    arr_add((setup.ionotify.handles), file_handle);

    reference.callback_func = xavaIONotifyCallbackWrapper;
    reference.context = setup.id;
    reference.path = setup.filename;
    reference.additional_data = &arr_back(setup.ionotify.handles);

    return xWatcher_appendFile(setup.ionotify.xwatcher_instance, &reference);
}

EXP_FUNC bool xavaIONotifyStart(const xava_ionotify ionotify)
{
    return xWatcher_start(ionotify.xwatcher_instance);
}

EXP_FUNC void xavaIONotifyKill(const xava_ionotify ionotify)
{
    xWatcher_destroy(ionotify.xwatcher_instance);
    arr_free(ionotify.handles);
}
