#include <x-watcher.h>

#include "shared/ionotify.h"
#include "shared.h"

// cringe but bare with me k
struct hax {
    void *original_data;
    void (*xava_ionotify_func)(XAVA_IONOTIFY_EVENT,
            const char *filename, int id, XAVA*);
};

static struct hax hax[100];
static size_t hax_count;

EXP_FUNC XAVAIONOTIFY xavaIONotifySetup(void) {
    hax_count = 0;
    return (XAVAIONOTIFY)xWatcher_create();
}

// before calling the acutal library, this function takes care of
// converting the xWatcher's event types to native ones
void __internal_xavaIONotifyWorkAroundDumbDecisions(XWATCHER_FILE_EVENT event,
        const char *name, int id, void* data) {
    struct hax *h = (struct hax*)data;
    XAVA_IONOTIFY_EVENT new_event = 0;

    switch(event) {
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
        case XWATCHER_FILE_RENAMED:            // ignoring because most likely breaks visualizer
        case XWATCHER_FILE_UNSPECIFIED:
            new_event = XAVA_IONOTIFY_NOTHING;
            break;
        default:
            new_event = XAVA_IONOTIFY_ERROR;
    }

    h->xava_ionotify_func(new_event, name, id, h->original_data);
}

EXP_FUNC bool xavaIONotifyAddWatch(XAVAIONOTIFYWATCHSETUP setup) {
    xWatcher_reference reference;

    // workaround in progress
    hax[hax_count].original_data      = setup->xava;
    xavaBailCondition(!setup->xava_ionotify_func,
        "BUG: IONotify function is NULL");
    hax[hax_count].xava_ionotify_func = setup->xava_ionotify_func;

    reference.callback_func   = __internal_xavaIONotifyWorkAroundDumbDecisions;
    reference.context         = setup->id;
    reference.path            = setup->filename;
    reference.additional_data = &hax[hax_count];

    // hax counter
    hax_count++;

    xavaBailCondition(hax_count > 100, "Scream at @nikp123 to fix this!");

    return xWatcher_appendFile(setup->ionotify, &reference);
}

EXP_FUNC bool xavaIONotifyStart(const XAVAIONOTIFY ionotify) {
    return xWatcher_start(ionotify);
}

EXP_FUNC void xavaIONotifyKill(const XAVAIONOTIFY ionotify) {
    xWatcher_destroy(ionotify);
}

