#pragma once
#include "inttypes.h"
#include "stdbool.h"
bool load_image_z80(char *file_name);
bool save_image_z80(char *file_name);
bool LoadScreenshot(char *file_name, bool open_file);
bool LoadScreenFromZ80Snapshot(char *file_name);