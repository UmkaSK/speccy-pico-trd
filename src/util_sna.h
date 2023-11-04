#pragma once
#include "inttypes.h"
#include "stdbool.h"

#define SNA_48K_SIZE 49179
#define SNA_128K_SIZE1 131103
#define SNA_128K_SIZE2 147487

bool load_image_sna(char *file_name);
bool LoadScreenFromSNASnapshot(char *file_name);