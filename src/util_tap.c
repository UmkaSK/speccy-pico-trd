#include "util_tap.h"
#include "util_sd.h"
#include <stdint.h>
#include <string.h>
#include <zx_emu/z80.h>
#include "pico/stdlib.h"
#include <ps2.h>
#include "zx_emu/zx_machine.h"
#include "screen_util.h"
#include <math.h>
#include "stdbool.h"
#include <VFS.h>
#include <ff.h>

#define ZX_RAM_PAGE_SIZE 0x4000
#define BUFF_PAGE_SIZE 0x200

extern volatile z80 cpu;
extern char sd_buffer[SD_BUFFER_SIZE];
extern char temp_msg[60];

/*
typedef struct TapeBlock{
	uint16_t Size;
	uint8_t Flag;
	uint8_t DataType;
	char NAME[11];
	uint32_t FPos;
} __attribute__((packed)) TapeBlock;
*/

uint8_t tapBlocksCount=0;

uint8_t TapeStatus;
uint8_t SaveStatus;
uint8_t RomLoading;
FIL f;
int tfd =-1; //tape file descriptor
size_t bytesRead;
size_t bytesToRead;

static uint8_t tapePhase;
static uint64_t tapeStart;
static uint32_t tapePulseCount;
static uint16_t tapeBitPulseLen;   
static uint8_t tapeBitPulseCount;     
static uint32_t tapebufByteCount;
static uint32_t tapeBlockByteCount;
static uint16_t tapeHdrPulses;
static uint32_t tapeBlockLen;
static uint8_t* tape;
static uint8_t tapeEarBit;
static uint8_t tapeBitMask; 

uint8_t tapeCurrentBlock;
size_t tapeFileSize;
uint32_t tapeTotByteCount;

char tapeBHbuffer[20]; //tape block header buffer


uint8_t __not_in_flash_func(TAP_Read)(){
//uint8_t TAP_Read(){
    uint64_t tapeCurrent = cpu.cyc - tapeStart;
    //printf("Tape PHASE:%X\n",tapePhase);
    switch (tapePhase) {
    case TAPE_PHASE_SYNC:
        if (tapeCurrent > TAPE_SYNC_LEN) {
            tapeStart=cpu.cyc;
            tapeEarBit ^= 1;
            tapePulseCount++;
            if (tapePulseCount>tapeHdrPulses) {
                tapePulseCount=0;
                tapePhase=TAPE_PHASE_SYNC1;
            }
        }
        break;
    case TAPE_PHASE_SYNC1:
        if (tapeCurrent > TAPE_SYNC1_LEN) {
            tapeStart=cpu.cyc;
            tapeEarBit ^= 1;
            tapePhase=TAPE_PHASE_SYNC2;
        }
        break;
    case TAPE_PHASE_SYNC2:
        if (tapeCurrent > TAPE_SYNC2_LEN) {
            tapeStart=cpu.cyc;
            tapeEarBit ^= 1;
            if (tape[tapebufByteCount] & tapeBitMask) tapeBitPulseLen=TAPE_BIT1_PULSELEN; else tapeBitPulseLen=TAPE_BIT0_PULSELEN;            
            tapePhase=TAPE_PHASE_DATA;
        }
        break;
    case TAPE_PHASE_DATA:
        if (tapeCurrent > tapeBitPulseLen) {
            tapeStart=cpu.cyc;
            tapeEarBit ^= 1;
            tapeBitPulseCount++;
            if (tapeBitPulseCount==2) {
                tapeBitPulseCount=0;
                tapeBitMask = (tapeBitMask >>1 | tapeBitMask <<7);
                if (tapeBitMask==0x80) {
                    tapebufByteCount++;
					tapeBlockByteCount++;
					tapeTotByteCount++;
					//printf("BUF:%d BLOCK:%d TOTAL:%d\n",tapebufByteCount,tapeBlockByteCount,tapeTotByteCount);
					if(tapebufByteCount>=BUFF_PAGE_SIZE){
						//printf("Read next buffer\n");
						tfd = sd_read_file(&f,sd_buffer,BUFF_PAGE_SIZE,&bytesRead);
                    	//printf("bytesRead=%d\n",bytesRead);
		                if (tfd!=FR_OK){
							printf("Error read SD\n");
							sd_close_file(&f);
							tap_loader_active = false;
							tapebufByteCount=0;
							//im_z80_stop = false;       
							TapeStatus=TAPE_STOPPED;
							return false;
						}
						//im_z80_stop = false;
						tapebufByteCount=0;
					}
					if(tapeBlockByteCount==(tapeBlockLen-2)){
						tapeTotByteCount+=2;
						//printf("Wait next block: %d\n",tapeTotByteCount);
						tapebufByteCount=0;
                        tapePhase=TAPE_PHASE_PAUSE;
                        tapeEarBit=false;
						if (tapeTotByteCount == tapeFileSize){
							//printf("Full Read TAPE_STOPPED 2\n");
							TapeStatus=TAPE_STOPPED;
							TAP_Rewind();
							tap_loader_active = false;
							return false;
						}
                        break;
					}					
                }
                if (tape[tapebufByteCount] & tapeBitMask) tapeBitPulseLen=TAPE_BIT1_PULSELEN; else tapeBitPulseLen=TAPE_BIT0_PULSELEN;
            }
        }
        break;
    case TAPE_PHASE_PAUSE:
        if (tapeTotByteCount < tapeFileSize) {
            if (tapeCurrent > TAPE_BLK_PAUSELEN) {
				tapeCurrentBlock++;
				sd_seek_file(&f,tap_blocks[tapeCurrentBlock].FPos);
				tapeBlockLen=tap_blocks[tapeCurrentBlock].Size + 2;
				bytesToRead = tapeBlockLen<BUFF_PAGE_SIZE ? tapeBlockLen : BUFF_PAGE_SIZE;
				tfd = sd_read_file(&f,sd_buffer,bytesToRead,&bytesRead);
				if (tfd != FR_OK){
					printf("Error read SD\n");
					sd_close_file(&f);
					tap_loader_active = false;
					tapebufByteCount=0;
					TapeStatus=TAPE_STOPPED;
					return false;
				}
				//printf("Block:%d Seek:%d Length:%d Read:%d\n",tapeCurrentBlock,tap_blocks[tapeCurrentBlock].FPos,tapeBlockLen,bytesRead);
                tapeStart=cpu.cyc;
                tapePulseCount=0;
                tapePhase=TAPE_PHASE_SYNC;
                tapebufByteCount=2;
				tapeBlockByteCount=0;
				//printf("Flag:%X, DType:%X \n",tap_blocks[tapeCurrentBlock].Flag,tap_blocks[tapeCurrentBlock].DataType);
                if (tap_blocks[tapeCurrentBlock].Flag) tapeHdrPulses=TAPE_HDR_SHORT; else tapeHdrPulses=TAPE_HDR_LONG;
            }
        } else {
			//printf("Full Read TAPE_STOPPED\n");
			TapeStatus=TAPE_STOPPED;
			TAP_Rewind();
			tap_loader_active = false;
			break;
		}
        return false;
    } 
  
    return tapeEarBit;
}


void __not_in_flash_func(TAP_Play)(){
//void TAP_Play(){
    switch (TapeStatus) {
    case TAPE_STOPPED:
		//TAP_Load(activefilename);
       	tapePhase=TAPE_PHASE_SYNC;
       	tapePulseCount=0;
       	tapeEarBit=false;
       	tapeBitMask=0x80;
       	tapeBitPulseCount=0;
       	tapeBitPulseLen=TAPE_BIT0_PULSELEN;
       	tapeHdrPulses=TAPE_HDR_LONG;
		sd_seek_file(&f,tap_blocks[tapeCurrentBlock].FPos);
		tapeTotByteCount = tap_blocks[tapeCurrentBlock].FPos;
		tapeBlockLen=tap_blocks[tapeCurrentBlock].Size + 2;
		bytesToRead = tapeBlockLen<BUFF_PAGE_SIZE ? tapeBlockLen : BUFF_PAGE_SIZE;
		tfd = sd_read_file(&f,sd_buffer,bytesToRead,&bytesRead);
		if (tfd != FR_OK){sd_close_file(&f);break;}
		//printf("Block:%d Seek:%d Length:%d Read:%d\n",tapeCurrentBlock,tap_blocks[tapeCurrentBlock].FPos,tapeBlockLen,bytesRead);		
       	tapebufByteCount=2;
		tapeBlockByteCount=0;
       	tapeStart=cpu.cyc;
       	TapeStatus=TAPE_LOADING;
       	break;
    case TAPE_LOADING:
       	TapeStatus=TAPE_PAUSED;
       	break;
    case TAPE_PAUSED:
        tapeStart=cpu.cyc;
        TapeStatus=TAPE_LOADING;
		break;
    }
}


void Init(){
	TapeStatus = TAPE_STOPPED;
	SaveStatus = SAVE_STOPPED;
	RomLoading = false;
}

bool TAP_Load(char *file_name){
	
	printf("Tap Load begin\n");
    TapeStatus = TAPE_STOPPED;
	printf("Tap FN:%s\n",file_name);
	tfd = sd_open_file(&f,file_name,FA_READ);
    printf("sd_open_file=%d\n",tfd);
	if (tfd!=FR_OK){sd_close_file(&f);return false;}
   	tapeFileSize = sd_file_size(&f);
    printf(".TAP Filesize %u bytes\n", tapeFileSize);
	tapBlocksCount=0;
	tapebufByteCount=0;
	tapeBlockByteCount=0;
	tapeTotByteCount=0;
	while (tapeTotByteCount<=tapeFileSize){
		tfd = sd_read_file(&f,tapeBHbuffer,14,&bytesRead);
		if (tfd != FR_OK){sd_close_file(&f);return false;}
		//printf(" Readbuf:%d\n", bytesRead);
		//printf(" pos:%d\n", tapeTotByteCount);
		TapeBlock* block = (TapeBlock*) &tapeBHbuffer;
		tap_blocks[tapBlocksCount].Size = block->Size;
		//printf(" block:%d, size:%d ",tapBlocksCount,block->Size);
		memset(tap_blocks[tapBlocksCount].NAME, 0, sizeof(tap_blocks[tapBlocksCount].NAME));
		if (block->Flag==0){
			memcpy(tap_blocks[tapBlocksCount].NAME,block->NAME,10);
			//printf(" header:%s", block->NAME);
		}
		tap_blocks[tapBlocksCount].Flag = block->Flag;
		tap_blocks[tapBlocksCount].DataType = block->DataType;
		//printf(" flag:%d, datatype:%d \n", block->Flag,block->DataType);
		tap_blocks[tapBlocksCount].FPos=tapeTotByteCount;
		tapeTotByteCount+=block->Size+2;
		sd_seek_file(&f,tapeTotByteCount);
		tapBlocksCount++;
		if(tapeTotByteCount>=tapeFileSize){
			break;
		}
		if(tapBlocksCount==TAPE_BLK_SIZE){
			break;
		}
	}
	tapebufByteCount=0;
	tapeBlockByteCount=0;
	tapeTotByteCount=0;
	tapeCurrentBlock=0;
	sd_seek_file(&f,0);
	tape = &sd_buffer;

	/*for(uint8_t j=0;j<tapBlocksCount;j++){
		printf(" block:%d, size:%d \n",j,tap_blocks[j].Size);
	}*/


	
    return true;
}

void TAP_Rewind(){
	TapeStatus=TAPE_STOPPED;
	printf("Tape Rewind\n");
	tapebufByteCount=0;
	tapeBlockByteCount=0;
	tapeCurrentBlock=0;	
	tapeTotByteCount=0;
};

void TAP_NextBlock(){
	printf("Tape NextBlock\n");
	tapeCurrentBlock++;
	if ((tapeCurrentBlock>=0)&&(tapeCurrentBlock<tapBlocksCount)){
		TapeStatus=TAPE_STOPPED;
		TAP_Play();
	}
	if (tapeCurrentBlock>tapBlocksCount){
		TAP_Rewind();
	}

};

void TAP_PrevBlock(){
	printf("Tape PrevBlock\n");
	tapeCurrentBlock--;
	if ((tapeCurrentBlock>=0)&&(tapeCurrentBlock<tapBlocksCount)){
		TapeStatus=TAPE_STOPPED;
		TAP_Play();
	}
	if (tapeCurrentBlock>tapBlocksCount){
		TAP_Rewind();
	}
};


/*

#define coef           (0.990) //0.993->
#define PILOT_TONE     (2168*coef) //2168
#define PILOT_SYNC_HI  (667*coef)  //667
#define PILOT_SYNC_LOW (735*coef)  //735
#define LOG_ONE        (1710*coef) //1710
#define LOG_ZERO       (855*coef)  //855

	#define PILOT_TONE     (2168) //2168
	#define PILOT_SYNC_HI  (667)  //667
	#define PILOT_SYNC_LOW (735)  //735
	#define LOG_ONE        (1710) //1710
	#define LOG_ZERO       (855)  //855
*/

bool LoadScreenFromTap(char *file_name){

	bool screen_found = false;

	memset(sd_buffer, 0, sizeof(sd_buffer));

	tfd = sd_open_file(&f,file_name,FA_READ);
    printf("sd_open_file=%d\n",tfd);
	if (tfd!=FR_OK){sd_close_file(&f);return false;}
   	tapeFileSize = sd_file_size(&f);
    printf(".TAP Filesize %u bytes\n", tapeFileSize);
	tapBlocksCount=0;
	tapebufByteCount=0;
	tapeBlockByteCount=0;
	tapeTotByteCount=0;
	while (tapeTotByteCount<=tapeFileSize){
		tfd = sd_read_file(&f,tapeBHbuffer,14,&bytesRead);
		if (tfd != FR_OK){sd_close_file(&f);return false;}
		//printf(" Readbuf:%d\n", bytesRead);
		//printf(" pos:%d\n", tapeTotByteCount);
		TapeBlock* block = (TapeBlock*) &tapeBHbuffer;
		tap_blocks[tapBlocksCount].Size = block->Size;
		//printf(" block:%d, size:%d ",tapBlocksCount,block->Size);
		memset(tap_blocks[tapBlocksCount].NAME, 0, sizeof(tap_blocks[tapBlocksCount].NAME));
		if (block->Flag==0){
			memcpy(tap_blocks[tapBlocksCount].NAME,block->NAME,10);
			//printf(" header:%s", block->NAME);
		}
		tap_blocks[tapBlocksCount].Flag = block->Flag;
		tap_blocks[tapBlocksCount].DataType = block->DataType;
		//printf(" flag:%d, datatype:%d \n", block->Flag,block->DataType);
		tap_blocks[tapBlocksCount].FPos=tapeTotByteCount;
		tapeTotByteCount+=block->Size+2;
		sd_seek_file(&f,tapeTotByteCount);
		tapBlocksCount++;
		if(tapeTotByteCount>=tapeFileSize){
			break;
		}
		if(tapBlocksCount==TAPE_BLK_SIZE){
			break;
		}
	}
	tapebufByteCount=0;
	tapeBlockByteCount=0;
	tapeTotByteCount=0;
	tapeCurrentBlock=0;
	sd_seek_file(&f,0);

	for (int8_t i=0;i<tapBlocksCount;i++){
		if (tap_blocks[i].Flag>0){
			if((tap_blocks[i].Size>=6914)&&(tap_blocks[i].Size<=6916)){
				sd_seek_file(&f,tap_blocks[i].FPos);
				tfd = sd_read_file(&f,sd_buffer,tap_blocks[i].Size,&bytesRead);
	        	printf("bytesRead=%d\n",bytesRead);
	        	if (tfd!=FR_OK){sd_close_file(&f);return false;}
		        ShowScreenshot(sd_buffer);
				screen_found = true;
				break;
			}
		}			
	}

	if (!screen_found){
		draw_text_len(18+FONT_W*14,16,"File contents:",COLOR_TEXT,COLOR_BACKGOUND,14);
		for (uint8_t i = 0; i < tapBlocksCount; i++){
			if (tap_blocks[i].Flag==0){
				memset(temp_msg, 0, sizeof(temp_msg));
				sprintf(temp_msg,"%s",tap_blocks[i].NAME);
			} else{
				memset(temp_msg, 0, sizeof(temp_msg));
				sprintf(temp_msg," DB %dKb",(tap_blocks[i].Size/1024));
			}
			draw_text_len(18+FONT_W*15,24+FONT_H*i,temp_msg,COLOR_TEXT,COLOR_BACKGOUND,10);
		}
	}
    memset(temp_msg, 0, sizeof(temp_msg));
	sprintf(temp_msg,"Type:.TAP");
	draw_text_len(18+FONT_W*14,208, temp_msg,COLOR_TEXT,COLOR_BACKGOUND,22);
	memset(temp_msg, 0, sizeof(temp_msg));
	sprintf(temp_msg,"FSize: %dKb",sd_file_size(&f)/1024);
	draw_text_len(18+FONT_W*14,216, temp_msg,COLOR_TEXT,COLOR_BACKGOUND,22);
	memset(temp_msg, 0, sizeof(temp_msg));
	strncpy(file_name,"A:/",3);
	strncpy(temp_msg,file_name,22);
	draw_text_len(18+FONT_W*14,224, temp_msg,COLOR_TEXT,COLOR_BACKGOUND,22);

	sd_close_file(&f);
	printf("\nAll loaded\n");
	memset(temp_msg, 0, sizeof(temp_msg));
    return true;
}

