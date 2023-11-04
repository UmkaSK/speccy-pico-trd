#include "PIO_program1.h"
#include "inttypes.h"

//#include "hardware/pio.h"



uint16_t pio_program1_instructions[] = {
    0x80a0, //  0: pull   block                      
    0xa027, //  1: mov    x, osr                     
    0x0028, //  2: jmp    !x, 8                      
    0xa042, //  3: nop                
    0x4008, //  4: in     pins, 8                    
    0xa042, //  5: nop           
    0x8020, //  6: push   block                      
    0x0003, //  7: jmp    3                          
            //     .wrap_target
    0x80a0, //  8: pull   block                      
    0x6008, //  9: out    pins, 8                    
            //     .wrap
};

const struct pio_program pio_program1 = {
    .instructions = pio_program1_instructions,
    .length = 10,
    .origin = -1,
};



