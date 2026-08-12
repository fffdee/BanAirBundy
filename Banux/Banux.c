/** @file Banux.c @brief Banux application facade implementation. */
#include "Banux.h"

static BanuxCallback_t g_setup_callback;
static BanuxCallback_t g_loop_callback;
static uint8_t g_started;

#if defined(__GNUC__)
__attribute__((weak)) void setup(void) {}
__attribute__((weak)) void loop(void) {}
__attribute__((weak)) void Banux_setup(void) {}
__attribute__((weak)) void Banux_loopCallback(void) {}
#else
void setup(void) {}
void loop(void) {}
void Banux_setup(void) {}
void Banux_loopCallback(void) {}
#endif

int Banux_begin(void)
{
    int ret;

    if (g_started)
        return 0;

    BG_Event_Init();
    ret = DrvFramework_FullInit();
    if (ret != 0)
        return ret;

    SysState_Init();
    ret = SysState_PowerOn();
    if (ret != 0)
        return ret;

    g_started = 1u;
    if (g_setup_callback)
        g_setup_callback();
    else {
        Banux_setup();
        setup();
    }
    return 0;
}

void Banux_loop(void)
{
    if (!g_started)
        return;

    SysState_Update();
    if (g_loop_callback)
        g_loop_callback();
    else {
        Banux_loopCallback();
        loop();
    }
}

void Banux_run(void)
{
    while (g_started)
        Banux_loop();
}

void Banux_setSetup(BanuxCallback_t callback)
{
    g_setup_callback = callback;
}

void Banux_setLoop(BanuxCallback_t callback)
{
    g_loop_callback = callback;
}
