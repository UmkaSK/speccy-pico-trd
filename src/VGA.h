#pragma once

#include "inttypes.h"
#include "stdbool.h"

#define beginVGA_PIN (6)
#define PIO_VGA (pio1)
#define SM_VGA (2)

#define PIO_VGA_conv (pio1)
#define SM_VGA_conv (0)
void setVGAWideMode(bool w_mode);
void startVGA();