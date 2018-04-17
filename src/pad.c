#include "pad.h"

#include <tamtypes.h>
#include <libpad.h>

typedef struct padState {
    u16 padPressed;
    u16 padHeld;
    u16 padOld;
    u16 initialDelay;       // Number of calls to padPoll() needed before considering a button to be held.
    u16 timeHeld;
    u16 timeHeldDelay;      // While a button is held, require a number of calls to padPoll() to be made before setting padHeld.
    struct padButtonStatus padStatus;
} padState_t;

static padState_t padState;
static char padBuff[256] __attribute__ ((aligned(64)));

void padInitialize()
{
    padInit(0);
    padPortOpen(0, 0, padBuff);
    padSetMainMode(0, 0, PAD_MMODE_DIGITAL, PAD_MMODE_LOCK);
    padState.padOld = 0xFFFF;
    padState.timeHeld = 0;
    padState.initialDelay = 18;
}

void padPoll(delayTime_t delayTime)
{
    padState.padHeld = 0;
    int state = padGetState(0, 0);
    while((state != PAD_STATE_STABLE) && (state != PAD_STATE_FINDCTP1))
        state = padGetState(0, 0);

    padRead(0, 0, &padState.padStatus);
    padState.padPressed = (0xFFFF ^ padState.padStatus.btns) & ~padState.padOld;
    padState.padOld = 0xFFFF ^ padState.padStatus.btns;

    // padState->padRapid will have an initial delay when a button is held
    if((0xFFFF ^ padState.padStatus.btns) && (0xFFFF ^ padState.padStatus.btns) == padState.padOld)
    {
        if(padState.timeHeld++ > padState.initialDelay && padState.timeHeld % delayTime == 0) // don't go too fast!
            padState.padHeld = (0xFFFF ^ padState.padStatus.btns);
        else
            padState.padHeld = padState.padPressed;
    }
    else
        padState.timeHeld = 0;
}

inline u32 padPressed()
{
    return padState.padPressed;
}

inline u32 padHeld()
{
    return padState.padHeld;
}
