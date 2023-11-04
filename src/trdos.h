#pragma once
#include "inttypes.h"
#include "stdbool.h"
#include <VFS.h>
#include <ff.h>
#include "zx_emu/zx_machine.h"


typedef struct {
    uint8_t RegStatus;
    uint8_t RegCom;
    int8_t RegTrack;
    uint8_t RegSect;
    uint8_t RegData;
    uint8_t System;
    int8_t StepDirect;
    uint8_t TrackReal[4];
    uint8_t reserved[3];
} DatVG;

DatVG VG;

//char Disks[4][160] = {"", "", "", ""};
char Disks[4][160];

void InitTRDOS();
void __not_in_flash_func(ChipVG93)(uint8_t OperIO, uint8_t *DataIO);
bool OpenTRDFile(char* sn);

void Set_INTRQ();
///////////////////////////////////////
#define rqDRQ   6
#define rqINTRQ 7

#define _BV(bit) (1 << (bit))


//	Requests &= ~_BV(rqDRQ); // CmdStartIdleForcent0 clear
//	Requests &= ~_BV(rqINTRQ);
// &= ~_BV(1 << 7);
