#pragma once
#include "inttypes.h"
#include "stdbool.h"

bool ReadCatalog(char *file_name, bool open_file);
void (*trdos_execute)(); // определение указателя на функцию
void trdos_nul (void);
void trdos_on (void);
void trdos_off (void);
