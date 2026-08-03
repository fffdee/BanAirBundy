/* Wrapper: compile the unified SDK USB FIFO source as part of bootloader/src */
#ifndef MAX
/* Bit-trick MAX: works for integer constant expressions, no ternary operator */
#define MAX(a, b) ((a) | (((a) ^ (b)) & -((a) < (b))))
#endif
#include "../../wireless_mic_unified_sdk/banux/05_component/audio/otg/otg_fifo.c"
