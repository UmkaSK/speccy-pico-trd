#pragma once
#include "stdio.h"

#define SCREEN_H (240)
#define SCREEN_W (320)
#define V_BUF_SZ (SCREEN_H*SCREEN_W/2)




//#define NUM_V_BUF (3)
#ifdef NUM_V_BUF
    extern  bool is_show_frame[NUM_V_BUF];
    extern int draw_vbuf_inx;
    extern int show_vbuf_inx;
#endif

extern uint8_t g_gbuf[];
extern uint8_t color_zx[16];

int	null_printf(const char *str, ...);
void g_delay_ms(uint delay);
#define G_PRINTF  printf
#define G_PRINTF_INFO  printf
#define G_PRINTF_DEBUG  printf
#define G_PRINTF_ERROR  printf





