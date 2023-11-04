#include "trdos.h"
#include "util_sd.h"
#include <stdint.h>
#include <string.h>
#include "pico/stdlib.h"
#include "stdbool.h"
#include <VFS.h>
#include <ff.h>

uint8_t FormatDatLen = 1;
bool Ready = true;
bool ReadIndexTrack = true;
uint8_t DskStatus = 0;


const uint8_t DskIndex[5] = {0, 1, 1, 0, 0};

uint8_t DiskBuf[256];
char DskFileName[160];
FIL hTRDFile;

uint8_t DskIndexCounter;
uint8_t DskCountDatLost;
int16_t CntData;
int16_t DskDataSize;
uint16_t CntReady;

int16_t GetFA(char *S) {
    return 0x20;
}

uint32_t DiskPosition(uint16_t Sect) {
    uint32_t res;
   res = (((VG.RegTrack << 5) | ((VG.System & 0x10) ^ 0x10) | Sect) << 8);
  
//  res = ((VG.RegTrack*2)*4096) + (Sect*256);
 //  printf("DiskPosition Sect=%d ret=%d\n",Sect++,res);
    return res;
}

void SetStateStep() {
    //printf("SetStateStep\n");
    if (VG.RegTrack < 0) {
        VG.TrackReal[VG.System & 0x03] = 0;
        VG.RegTrack = 0;
    }
    if (VG.RegTrack > 0x80) { ///0x4F количество дорожек на стороне
        VG.TrackReal[VG.System & 0x03] = 0x80;
        VG.RegTrack = 0x80;
    }

    VG.RegStatus = DskStatus & 0x40;
    if (VG.RegCom & 0x08) {
        VG.RegStatus |= 0x20;
    }
    if (VG.RegTrack == 0) {
        VG.RegStatus |= 0x04;
    }
}

void OpCom() {
    //printf("OpCom\n");
    DskCountDatLost = 0x00;
    CntData = 0x0000;
    VG.RegStatus = 0x03;
    if (!Ready) {
        VG.RegStatus = 0x01;
        CntReady = 0x10;
    }
}

void StepRear() {
    //printf("StepRear\n");
    if (VG.RegTrack > 0) {
        VG.RegTrack--;
        VG.TrackReal[VG.System & 0x03] = VG.RegTrack;
    }
    VG.StepDirect = -1;
    SetStateStep();
}

void OpReadSector() // чтение сектора
{
    size_t numread;
    uint8_t fd;
    uint32_t Position;

   


    if ((VG.RegSect > 0x10) || (VG.RegTrack != VG.TrackReal[VG.System & 0x03]) || !(DskStatus & 0x04)) {
        VG.RegStatus = 0x10; // OpNotFound
      printf("TRC=%d SEC=%d not found\n",(int)VG.RegTrack,VG.RegSect );  
        return;
    }
 
    Position = DiskPosition(VG.RegSect - 1);
   
    sd_seek_file(&hTRDFile,Position);
    memset(DiskBuf, 0, sizeof(DiskBuf));
    fd = sd_read_file(&hTRDFile,DiskBuf,sizeof(DiskBuf),&numread);
//if (Position == 2048+256)
  // if (Position == 2048) {
  //      DiskBuf[227] = 16; // BUG !!! at pos $8e3 must be $10 ??
  //  }
 //  printf("SIDE=%X  TRC=%d SEC=%d BANK=%X\n",((VG.System & 0x10) ^ 0x10)>>4,VG.RegTrack*2,VG.RegSect, zx_machine_get_7ffd_lastOut() );
 //  printf("OpReadSector pos=%X numread=%d\n",Position,numread);
   
   
  //  for (int i = 0; i < 256; i++) {
 //       printf("%02X",DiskBuf[i]);
  //  }
 //   printf("\n");
   

    if (numread != sizeof(DiskBuf)) {
        VG.RegStatus = 0x08; // OpContSum
    } else {
        DskDataSize = 0xFF;
        VG.RegData = DiskBuf[0];
        OpCom();
    }
}

void OpWriteSector() {
    if (DskStatus & 0x40) {
        VG.RegStatus = 0x40; // OpWrProt
        return;
    }

   // printf("WRITE %X\n",VG.RegTrack);
    if ((VG.RegSect > 0x10) || (VG.RegTrack != VG.TrackReal[VG.System & 0x03]) || !(DskStatus & 0x04)) {
        VG.RegStatus = 0x10; // OpNotFound
    } else {
        DskDataSize = 0x0100;
        OpCom();
    }
}

void __not_in_flash_func(ChipVG93)(uint8_t OperIO, uint8_t *DataIO){
    size_t numread;
    size_t numwritten;
    uint16_t i;
    uint16_t attr;
    uint16_t IntrqDrq;
    uint8_t Intrq;
    uint8_t Drq;
    uint8_t fd;

    //printf("OperIO=%d DataIO=%d\n",OperIO,*DataIO);

    if ((VG.RegStatus & 0x01) > 0) {
        DskCountDatLost--;
        if (DskCountDatLost == 0) VG.RegStatus = 0x04;
    }
    if (CntReady > 0) {
        CntReady--;
        if (CntReady == 0) VG.RegStatus = 0x03;
    }

    switch (OperIO) {
//-------------------- #1F in
        case 0x00:

            *DataIO = VG.RegStatus;
            if ((VG.RegCom & 0x80) > 0) return;
            DskIndexCounter--;
       
            if ((DskIndexCounter & 0x0E) > 0) *DataIO |= 0x02;
            break;
 //----------- #1F
        case 0x01:

            if ((VG.RegStatus & 0x01) == 0) {

                if ((*DataIO & 0xF0) == 0xD0) return;
 
                VG.RegCom = *DataIO;

                switch (VG.RegCom >> 4) {
                    case 0x00:
                        VG.TrackReal[VG.System & 0x03] = 0;
                        VG.RegTrack = 0;
                        VG.StepDirect = -1;
                        SetStateStep();
                        break;

                    case 0x01:
                        VG.TrackReal[VG.System & 0x03] = VG.RegData;
                        VG.RegTrack = VG.RegData;
                        if ((VG.RegData - VG.RegTrack) < 0) VG.StepDirect = -1;
                        if ((VG.RegData - VG.RegTrack) > 0) VG.StepDirect = 1;
                        SetStateStep();
                        break;

                    case 0x02:
                    case 0x03:
                    case 0x04:
                    case 0x05:
                        if (((VG.RegCom >> 4) == 2 || (VG.RegCom >> 4) == 3) && (VG.StepDirect == -1)) {
                            StepRear();
                            return;
                        }
                        if (VG.RegTrack > 0) {
                            VG.RegTrack++;
                            VG.TrackReal[VG.System & 0x03] = VG.RegTrack;
                        }
                        VG.StepDirect = 1;
                        SetStateStep();
                        break;

                    case 0x06:
                    case 0x07:
                        StepRear();
                        break;

                    case 0x08:
                    case 0x09:
                        OpReadSector();
                        break;

                    case 0x0A:
                    case 0x0B:
                        OpWriteSector();
                        break;

                    case 0x0C:
                        DskDataSize = 0x0005;
                        VG.RegData = 1;
                        if (ReadIndexTrack) VG.RegData = VG.RegTrack;
                        OpCom();
                        break;


                    case 0x0E:
                        break;

                    case 0x0F:
                        if ((DskStatus & 0x40) > 0)
                            VG.RegStatus = 0x40; //OpWrProt
                        else {
                            DskDataSize = FormatDatLen;
                            OpCom();
                        }
                        break;
                }
            }
            break;//case 0x01:
//  #1F end-------------------------------------------
//----------------------------------------------------
//   #3F  Номер дорожки
        case 0x02: // чтение
            *DataIO = VG.RegTrack;
            break;

        case 0x03: // запись
            if ((VG.RegStatus & 0x01)==0) VG.RegTrack = *DataIO;
            break;
//-----------------------------------------------------------------------
// #5f Номер сектора
        case 0x04: // чтение
            *DataIO = VG.RegSect;
            break;

        case 0x05: // запись
            if ((VG.RegStatus & 0x01)==0) VG.RegSect = *DataIO;
            break;
//------------------------------------------------------------------------
//  #7f  
         case 0x06:
            if ((VG.RegStatus & 0x01) > 0) {
                switch (VG.RegCom >> 4) {
                    case 0x08:
                    case 0x09: {
                        *DataIO = VG.RegData;
                        DskCountDatLost = 0x10;
                        if (CntData < DskDataSize) {
                            CntData++;
                            VG.RegData = DiskBuf[CntData];
                        } else {
                            if ((VG.RegCom & 0x10) > 0) {
                                VG.RegSect++;
                                OpReadSector();
                            } else {
                                VG.RegStatus = 0x00; // OpOk
                            }
                        }
                        break;
                    }
                    case 0x0C: {
                        *DataIO = VG.RegData;
                        DskCountDatLost = 0x10;
                        if (CntData < DskDataSize) {
                            VG.RegData = DskIndex[CntData];
                            CntData++;
                        } else {
                            VG.RegStatus = 0x00; // OpOk
                        }
                        break;
                    }
                }
            } else {
                *DataIO = VG.RegData;
            }
            break;
//---------------------------------------------------------------------
        case 0x07:// out #7f
          if ((VG.RegStatus & 0x01) > 0) {
                switch (VG.RegCom >> 4) {
                    case 0x0A:
                    case 0x0B: {
                        DiskBuf[CntData] = *DataIO;
                        VG.RegData = *DataIO;
                        CntData++;
                        DskCountDatLost = 0x10;
                        if (CntData >= DskDataSize) {
                            sd_seek_file(&hTRDFile,DiskPosition(VG.RegSect - 1));
                            fd = sd_write_file(&hTRDFile,DiskBuf,sizeof(DiskBuf),&numwritten);
                            if (numwritten != sizeof(DiskBuf)) {
                                VG.RegStatus = 0x08; // OpContSum
                                return;
                            }
                            if ((VG.RegCom & 0x10) > 0) {
                                VG.RegSect++;
                                OpWriteSector();
                            } else {
                                VG.RegStatus = 0x00; // OpOk
                            }
                        }
                        break;
                    }
//-------------------------------------------------------------------------------
// #FF                    
                    case 0x0F: {
                        VG.RegData = *DataIO;
                        DskCountDatLost = 0x10;
                        CntData++;
                        if (CntData >= DskDataSize) {
                            memset(DiskBuf, 0, sizeof(DiskBuf));
                            if ((DskStatus & 0x04) > 0) {
                                sd_seek_file(&hTRDFile,DiskPosition(0));
                                for (int i = 0; i < 0x10; i++) {
                                    fd = sd_write_file(&hTRDFile,DiskBuf,sizeof(DiskBuf),&numwritten);
                                }
                            } else {
                                fd = sd_open_file(&hTRDFile,DskFileName,FA_WRITE);
                                if (fd!=FR_OK) {
                                    VG.RegStatus = 0x00; // OpOk
                                    return;
                                }
                                for (int i = 0; i < 0xA00; i++) {
                                    fd = sd_write_file(&hTRDFile, DiskBuf, sizeof(DiskBuf), &numwritten);
                                }
                                DskStatus = DskStatus | 0x04;
                                //sd_close_file(&hTRDFile);
                            }
                            VG.RegStatus = 0x00; // OpOk
                        }
                        break;
                    }
                }
            } else {
                VG.RegData = *DataIO;
            }
            break;
//---------------------------------------
// in ff
        case 0x08:
            IntrqDrq = 0xBF;
            if ((VG.RegStatus & 0x01) > 0) {
                IntrqDrq = 0x3F;
            }
            if ((VG.RegStatus & 0x02) > 0) {
                IntrqDrq = 0x7F;
            }
            *DataIO = IntrqDrq;
            break;

        case 0x09:
            if (((VG.RegStatus & 0x01) > 0) && ((VG.RegCom & 0x80) > 0)) {
                VG.RegStatus = 0x08; // OpContSum
                return;
            }
            VG.System = *DataIO;
            if (((VG.System ^ DskStatus) & 0x03) > 0) {
                ChipVG93(0x0B, DataIO);
                ChipVG93(0x0A, DataIO);
            }
            break;

        case 0x0A:
            if ((DskStatus & 0x80) > 0) return;
                
           
            DskStatus = VG.System & 0x03;
            //DskFileName = Disks[DskStatus];
            strcpy(DskFileName,Disks[DskStatus]);

            attr = GetFA(DskFileName);
            if (attr != -1) {
                attr &= 0x01;
                DskStatus |= attr << 6;
                fd = sd_open_file(&hTRDFile,DskFileName,FA_READ);
                if (fd==FR_OK) {
                    DskStatus =(DskStatus | 0x04);
                    //sd_close_file(&hTRDFile);
                }
            }
            DskStatus = (DskStatus | 0x80);
            break;

        case 0x0B:
            if ((DskStatus & 0x80) == 0) {
                return;
            }
            if ((DskStatus & 0x04) > 0) {
                sd_close_file(&hTRDFile);
            }
            DskStatus = 0x00;
            break;

        case 0x0C:
            VG.RegStatus = 0x24;
            VG.RegCom = 0x00;
            VG.RegTrack = 0x00;
            VG.RegSect = 0x01;
            VG.RegData = 0x00;
            VG.System = 0x3C;
            VG.StepDirect = 0xFF;
            if (((DskStatus & 0x80)>0) && ((DskStatus & 0x03)>0)) {
               ChipVG93(0xB, DataIO);
               ChipVG93(0xA, DataIO);
            }
            break;

    }
}

bool OpenTRDFile(char* sn){
    strcpy(Disks[0],sn);
    //printf("Disks[0]=%s\n",Disks[0]);
    sd_open_file(&hTRDFile,sn,FA_READ);   
    return true;
}

void InitTRDOS(){
    VG.RegStatus=0;
    VG.RegCom=0;
    VG.RegTrack=0;
    VG.RegSect=1;
    VG.RegData=0;
    VG.System=0;
    VG.StepDirect=0;
    VG.TrackReal[0]=0;
    VG.TrackReal[1]=0;
    VG.TrackReal[2]=0;
    VG.TrackReal[3]=0;
    VG.reserved[0]=0;
    VG.reserved[1]=0;    
    VG.reserved[2]=0;
}
////////////////////////////////////////////
