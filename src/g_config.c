#include "g_config.h"

#ifdef NUM_V_BUF
    uint8_t g_gbuf[V_BUF_SZ*NUM_V_BUF];
    bool is_show_frame[NUM_V_BUF];
    int draw_vbuf_inx=0;
    int show_vbuf_inx=0;

#else
    uint8_t g_gbuf[V_BUF_SZ];
#endif
int	null_printf(const char *str, ...){return 0;};

void g_delay_ms(uint delay)
{
    sleep_ms(delay);
    
};



uint8_t color_zx[16]={0b00000000,
                      0b00000010,
                      0b00001000,
                      0b00001010,
                      0b00100000,
                      0b00100010,
                      0b00101000,
                      0b00101010,
                      0b00000000,
                      0b00000011,
                      0b00001100,
                      0b00001111,
                      0b00110000,
                      0b00110011,
                      0b00111100,
                      0b00111111 
                      };


