#pragma once
#include "inttypes.h"
#include "stdbool.h"
#include <VFS.h>
#include <ff.h>

// Tape status definitions
#define TAPE_STOPPED 0
#define TAPE_LOADING 1
#define TAPE_PAUSED 2

// Saving status
#define SAVE_STOPPED 0
#define TAPE_SAVING 1

// Tape phases
#define TAPE_PHASE_SYNC 1
#define TAPE_PHASE_SYNC1 2
#define TAPE_PHASE_SYNC2 3
#define TAPE_PHASE_DATA 4
#define TAPE_PHASE_PAUSE 5
#define TAPE_PHASE_NEED_STOP 6

// Tape sync phases lenght in microseconds
#define TAPE_SYNC_LEN 2168 // 620 microseconds for 2168 tStates (48K)
#define TAPE_SYNC1_LEN 667 // 190 microseconds for 667 tStates (48K)
#define TAPE_SYNC2_LEN 735 // 210 microseconds for 735 tStates (48K)

#define TAPE_HDR_LONG 8063   // Header sync lenght in pulses
#define TAPE_HDR_SHORT 3223  // Data sync lenght in pulses

#define TAPE_BIT0_PULSELEN 855 // tstates = 244 ms, lenght of pulse for bit 0
#define TAPE_BIT1_PULSELEN 1710 // tstates = 488 ms, lenght of pulse for bit 1

//#define TAPE_BLK_PAUSELEN 3500000UL // 1 second of pause between blocks
#define TAPE_BLK_PAUSELEN 1750000UL // 1/2 second of pause between blocks
//#define TAPE_BLK_PAUSELEN 87500UL // 1/4 second of pause between blocks

#define TAPE_BLK_SIZE 30

typedef struct TapeBlock{
	uint16_t Size;
	uint8_t Flag;
	uint8_t DataType;
	char NAME[11];
	uint32_t FPos;
} __attribute__((packed)) TapeBlock;

bool tap_loader_active;
uint8_t tape_autoload_status;
uint16_t tap_block_position;

//char tapeFileName[160];
uint8_t TapeStatus;
uint8_t SaveStatus;
uint8_t RomLoading;
FIL f;
size_t file_pos;
TapeBlock tap_blocks[TAPE_BLK_SIZE];

uint8_t tapeCurrentBlock;
size_t tapeFileSize;
uint32_t tapeTotByteCount;

uint8_t __not_in_flash_func(TAP_Read)();
void __not_in_flash_func(TAP_Play)();
//uint8_t TAP_Read();
//void TAP_Play();
void Init();
bool TAP_Load(char *file_name);
void TAP_Rewind();
void TAP_NextBlock();
void TAP_PrevBlock();


/*
void tap_tick_cpu_delay(uint16_t ticks);
void tap_pilot(unsigned long time);
void tap_sync();
void tap_bit(uint8_t tap_bit);
void tap_send_byte(uint8_t tap_byte);
bool load_tap(char *file_name);
*/
bool LoadScreenFromTap(char *file_name);
