#pragma once

#include "inttypes.h"
#include "hardware/pio.h"

//определение PIO для программы и номера SM для разных подзадач
#define PIO_p1 (pio1)
//номера SM задают номер подпрограммы(можно переделать, жёстко зафиксировав при активации PIO, но пока так)
#define SM_out (0)
#define SM_in_ps2 (3)


#define pio_program1_wrap_target 8
#define pio_program1_wrap 9


void pio_prog1_init();//параметр инициализации программы PIO prog1
extern uint16_t pio_program1_instructions[];
extern const struct pio_program pio_program1;