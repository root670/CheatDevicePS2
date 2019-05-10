#ifdef __PS1__
    #include <stddef.h>
#endif // __PS1__

#include "pad.h"
#include "types.h"

typedef struct padState {
    u16 padPressed;
    u16 padHeld;
    u16 padOld;
    u16 initialDelay;       // Number of calls to padPoll() needed before considering a button to be held.
    u16 timeHeld;
    u16 timeHeldDelay;      // While a button is held, require a number of calls to padPoll() to be made before setting padHeld.
    
    #ifdef __PS2__
    struct padButtonStatus padStatus;
    #elif __PS1__
    unsigned short padStatus;
    #endif
} padState_t;

padState_t padState;

#ifdef __PS2__
    static char padBuff[256] __attribute__ ((aligned(64)));
#endif // __PS2__

void padInitialize()
{
#ifdef __PS2__
    padInit(0);
    padPortOpen(0, 0, padBuff);
    padSetMainMode(0, 0, PAD_MMODE_DIGITAL, PAD_MMODE_LOCK);
#endif // __PS2__

    padState.padOld = 0xFFFF;
    padState.timeHeld = 0;
    padState.initialDelay = 18;
}

static inline void getPadStatus()
{
#ifdef __PS2__
    int state = padGetState(0, 0);
    while((state != PAD_STATE_STABLE) && (state != PAD_STATE_FINDCTP1))
        state = padGetState(0, 0);

    padRead(0, 0, &padState.padStatus);

#elif __PS1__
    PSX_ReadPad(&padState.padStatus, NULL);
#endif
}

static inline u32 getButtons()
{
#ifdef __PS2__
    return 0xFFFF ^ padState.padStatus.btns;
#elif __PS1__
    return padState.padStatus;
#endif
}

void padPoll(delayTime_t delayTime)
{
    getPadStatus();

    padState.padHeld = 0;
    padState.padPressed = getButtons() & ~padState.padOld;
    padState.padOld = getButtons();

    // padState->padRapid will have an initial delay when a button is held
    if(getButtons() && getButtons() == padState.padOld)
    {
        if(padState.timeHeld++ > padState.initialDelay && padState.timeHeld % delayTime == 0) // don't go too fast!
            padState.padHeld = getButtons();
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
