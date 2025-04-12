#include <assert.h>

#include "shared.h"

int main() {
    XG_EVENT *stack = newXAVAEventStack();
    assert(pendingXAVAEventStack(stack) == 0);
    pushXAVAEventStack(stack, XAVA_QUIT);
    assert(pendingXAVAEventStack(stack) != 0);

    assert(isEventPendingXAVA(stack, XAVA_RELOAD) != true);
    assert(isEventPendingXAVA(stack, XAVA_QUIT) == true);

    XG_EVENT poppedEvent = popXAVAEventStack(stack);
    assert(poppedEvent == XAVA_QUIT);

    destroyXAVAEventStack(stack);

    return 0;
}
