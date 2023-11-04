#pragma once
#include "inttypes.h"
#include <stdio.h>
#include "kb_u_codes.h"

#define beginPS2_PIN (0)
//#define beginPS2_PIN (14)

void  ps2_proc (uint8_t val);
extern uint8_t* zx_keyboard_state;
extern kb_u_state kb_st_ps2;
//static inline uint8_t get_scan_code(void);
bool decode_PS2();
void start_PS2_capture();
