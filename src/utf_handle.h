#pragma once
#include "inttypes.h"
#include "stdbool.h"
#include "strings.h"
int convert_utf8_to_windows1251(const char* utf8, char* windows1251, size_t n);