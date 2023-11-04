#include "util_sna.h"
#include "util_sd.h"
#include <VFS.h>
#include <fcntl.h>
#include <string.h>
#include <zx_emu/z80.h>
#include "zx_emu/aySoundSoft.h"
#include "zx_emu/zx_machine.h"
#include "screen_util.h"

/*
	Формат файлов .sna
	
	Материал из Энциклопедии ZX Spectrum.
	
	Образ программы в  формате  .sna  (от  английского  snapshot) -
	один из наиболее широко поддерживаемых форматов  для  сохранения
	состояния работающей программы для последующего её выполнения.
	
	Версия для 48K
	Основным недостатком этого формата является использование  стэка
	для сохранения программного счётчика (содержимого  регистра  PC),
	для последующего восстановления работы программ инструкцией RETN.
	В общем случае это не сказывается на  работе  программы,  однако
	если вершина стэка в момент сохранения образа находилась  в  об-
	ласти ПЗУ или критически близко к исполняемому коду, последствия
	могут быть фатальными для работаюшей программы.
	
	Смещение Размер   Описание
	---------------------------
	0		1		Регистр I.
	1		2		Регистровая пара HL'.
	3		2		Регистровая пара DE'.
	5		2		Регистровая пара BC'.
	7		2		Регистровая пара AF'.
	9		2		Регистровая пара HL.
	11		2		Регистровая пара DE.
	13		2		Регистровая пара BC.
	15		2		Регистровая пара IY.
	17		2		Регистровая пара IX.
	19		1		Состояние прерываний. Бит 2 содержит состояние триггера IFF2, бит 1 - IFF1 (0=DI, 1=EI).
	20		1		Регистр R.
	21		2		Регистровая пара AF.
	23		2		Указатель на вершину стэка (SP).
	25		1		Режим прерываний: 0=IM0, 1=IM1, 2=IM2.
	26		1		Цвет бордюра, 0-7.
	27		49152	Содержимое памяти с адреса 16384 (4000h).
	
	Версия для 128K
	Этот формат расширяет предыдущий, позволяя  хранить  расширенную
	память ZX Spectrum 128 и +3, а также  решает  проблему  хранения
	регистра PC: теперь он сохраняется не в стэке,  а  в  специально
	отведённой области в файле.
	
	Полный формат файла:
	
	Смещение Размер   Описание
	---------------------------
	0		27	   Заголовок SNA, см. выше.
	27	   16384	Страница памяти #5.
	16411	16384	Страница памяти #2.
	32795	16384	Активная страница памяти. Если активна страни-
	ца #2 или #5 -  она,  фактически,  хранится  в
	файле дважды.
	49179	2		Содержимое регистра PC.
	49181	1		Содержимое порта 7FFDh.
	49182	1		1 если активно ПЗУ TR-DOS, 0 - если нет.
	49183	16384	Оставшиеся страницы памяти.
	
	Общая длина файла составляет 131103 или 147487 байтов.  Дополни-
	тельные страницы памяти хранятся в порядке возврастания, за  ис-
	ключением страниц #2, #5 и текущей активной.
	
	Получено с http://zx.org.ru/db/.sna
	
*/


//#define DUMP_PAGES_V1
//#define DUMP_BUFFER

#define ZX_RAM_PAGE_SIZE 0x4000

extern uint8_t RAM[ZX_RAM_PAGE_SIZE*8]; //Реальная память куском 128Кб
extern z80 cpu;
extern uint8_t zx_RAM_bank_active;
extern uint8_t* zx_cpu_ram[4];//Адреса 4х областей памяти CPU при использовании страниц
extern uint8_t* zx_ram_bank[8];
extern uint8_t* zx_rom_bank[4];//Адреса 4х областей ПЗУ (48к 128к TRDOS и резерв для какого либо режима(типа тест))
extern uint8_t* zx_video_ram;

extern bool zx_state_48k_MODE_BLOCK;
extern uint8_t zx_Border_color;

//extern uint8_t zx_machine_last_out_7ffd;
extern uint8_t zx_machine_last_out_fffd;

extern char sd_buffer[SD_BUFFER_SIZE];
extern int last_error;

/*
	0		1		Регистр I.
	1		2		Регистровая пара HL'.
	3		2		Регистровая пара DE'.
	5		2		Регистровая пара BC'.
	7		2		Регистровая пара AF'.
	9		2		Регистровая пара HL.
	11		2		Регистровая пара DE.
	13		2		Регистровая пара BC.
	15		2		Регистровая пара IY.
	17		2		Регистровая пара IX.
	19		1		Состояние прерываний. Бит 2 содержит состояние триггера IFF2, бит 1 - IFF1 (0=DI, 1=EI).
	20		1		Регистр R.
	21		2		Регистровая пара AF.
	23		2		Указатель на вершину стэка (SP).
	25		1		Режим прерываний: 0=IM0, 1=IM1, 2=IM2.
	26		1		Цвет бордюра, 0-7.
*/

typedef struct FileHeader{
	uint8_t IR;  //0
	uint8_t L_Dash; //1
	uint8_t H_Dash; //2
	uint8_t E_Dash; //3
	uint8_t D_Dash; //4
	uint8_t C_Dash; //5
	uint8_t B_Dash; //6
	uint8_t A_Dash; //7
	uint8_t F_Dash; //8
	uint8_t L; //9  HL register pair
	uint8_t H; //10  HL register pair
	uint8_t E; //11
	uint8_t D; //12
	uint8_t C; //13  BC register pair (LSB, i.e. C, first)
	uint8_t B; //14  BC register pair (LSB, i.e. C, first)
	uint16_t IY; //15<<16
	uint16_t IX; //17<<18
	uint8_t IFF; //19
	uint8_t RR;  //20
	uint8_t F; //21  F register
	uint8_t A; //22  A register
	uint16_t SP; //24<<23 Stack pointer
	uint8_t IMode; //25
	uint8_t Border; //26
} __attribute__((packed)) FileHeader;


int fd;					    // индикатор ошибки чтения файла
char buf[10];			    // временный буфер
char header_buf[87];		// буфер для чтения заголовка
uint8_t last_out_7ffd;      // порт банков памяти

uint8_t read_byte_48k(uint16_t addr){ // Чтение памяти
	if (addr<16384) return zx_cpu_ram[0][addr];
	if (addr<32768) return zx_cpu_ram[1][addr-16384];
	if (addr<49152) return zx_cpu_ram[2][addr-32768];
	return zx_cpu_ram[3][addr-49152];	
}

static inline uint16_t read_word_48k(uint16_t addr) {
	return (read_byte_48k(addr + 1) << 8) |
			read_byte_48k(addr);
}

void readCPUstateSNA(FileHeader* header,uint8_t im_hw){
	printf("readCPUstate begin\n");
	
	cpu.a = header->A;
	
	cpu.sf = (header->F & 0b10000000);
	cpu.zf = (header->F & 0b01000000);
	cpu.yf = (header->F & 0b00100000);
	cpu.hf = (header->F & 0b00010000);
	cpu.xf = (header->F & 0b00001000);
	cpu.pf = (header->F & 0b00000100);
	cpu.nf = (header->F & 0b00000010);
	cpu.cf = (header->F & 0b00000001);
	
	cpu.b = header->B; cpu.c = header->C;
	cpu.h = header->H; cpu.l = header->L;
	
	cpu.sp = header->SP;
	cpu.i = header->IR;
	cpu.r = header->RR;
	//cpu.r = cpu.r|(((header->Flags1&1)<<7)&0b10000000);
	
	zx_Border_color=header->Border;
	printf("Border color: %02x\n",zx_Border_color);
	
	cpu.d   = header->D; cpu.e   = header->E;	
	
	cpu.b_  = header->B_Dash; cpu.c_  = header->C_Dash;	
	cpu.d_  = header->D_Dash; cpu.e_  = header->E_Dash;	
	cpu.h_  = header->H_Dash; cpu.l_  = header->L_Dash;
	
	cpu.a_  = header->A_Dash;
	cpu.f_  = header->F_Dash;
	
	
	cpu.ix  = header->IX;
	cpu.iy  = header->IY;
	
	cpu.iff1 = (header->IFF & 0b00000001);
	cpu.iff2 = (header->IFF & 0b00000010);
	

	cpu.interrupt_mode  = header->IMode; //Биты 0-1: режим прерываний (0-2)
	printf("cpu.interrupt_mode %d\n",cpu.interrupt_mode);
	cpu.int_pending = 0;


	printf("im_hw: %d \n",im_hw);


	printf("last_out_7ffd: %02X\n", last_out_7ffd);
	
	if (im_hw == 48) last_out_7ffd = 0x30;//last_out_7ffd | 0b00110000; // ??

	zx_RAM_bank_active = last_out_7ffd & 0x7;
	zx_cpu_ram[3]=zx_ram_bank[zx_RAM_bank_active]; // 0xC000 - 0x7FFF 
	
	if (last_out_7ffd &  8) zx_video_ram=zx_ram_bank[7];  else zx_video_ram  = zx_ram_bank[5];
	if (last_out_7ffd & 16) zx_cpu_ram[0]=zx_rom_bank[1]; else zx_cpu_ram[0] = zx_rom_bank[0]; 
	//if ((last_out_7ffd & 32)&& (im_hw==48)) zx_state_48k_MODE_BLOCK=true; // 6bit = 1 48k mode block
	if (last_out_7ffd & 32) zx_state_48k_MODE_BLOCK=true; // 6bit = 1 48k mode block // && (im_hw==48)
	
	printf("last_out_7ffd modified: %02X\n", last_out_7ffd);
	
	printf("readCPUstate end\n");
}

/*  memory dump
	printf("[%04X] Page 5 Used bytes[%04X]\n",5*ZX_RAM_PAGE_SIZE,usedBytes);
	ptr=0;
	do{
	printf("[%04X]",5*ZX_RAM_PAGE_SIZE+ptr);
	for (uint8_t col=0;col<16;col++){
	printf("\t%02X",RAM[5*ZX_RAM_PAGE_SIZE+ptr]);
	ptr++;
	}
	printf("\n");
	} while(ptr<0x4000);
	printf("\n");
*/

bool load_image_sna(char *file_name){
	FIL f;
	int res;
	size_t bytesRead;
	UINT bytesToRead;	
	int fd=0;
	uint16_t ptr=0;
	uint16_t usedBytes=0;
	uint8_t im_hw = 0;			  //Тип оборудования для V2 V3
	uint8_t last_out_7ffd;
	
	printf("load_image_sna\n");
	
	memset(header_buf, 0, sizeof(header_buf));
	
	fd = sd_open_file(&f,file_name,FA_READ);
	//printf("sd_open_file=%d\n",fd);
	if (fd!=FR_OK){sd_close_file(&f);return false;}
	
	
	if (sd_file_size(&f)<SNA_48K_SIZE){
		sd_close_file(&f);return false;
	}
	if (sd_file_size(&f)>SNA_128K_SIZE2){
		sd_close_file(&f);return false;
	} 
	
	//printf("Begin read\n");
	bytesToRead = 27;
	
	fd = sd_read_file(&f,header_buf,bytesToRead,&bytesRead);
	//printf("bytesToRead=%d, bytesRead=%d\n",bytesToRead,bytesRead);
	if (fd!=FR_OK){sd_close_file(&f);return false;}
	if (bytesRead != bytesToRead){sd_close_file(&f);return false;}
	
	FileHeader* header = (FileHeader*)header_buf;
	printf("Header assign\n");
	
	
	
	//printf("Header PC=%04X\n",header->PC);
	//printf("Header A=%02X\n",header->A);
	
	if (sd_file_size(&f) == SNA_48K_SIZE){
		// version 1
		im_hw = 48;
		zx_RAM_bank_active = 0;
		//readCPUstate(header);
		printf("*V48\n");
	} else if (sd_file_size(&f) > SNA_48K_SIZE){
		im_hw = 128;
		zx_RAM_bank_active = 0;
		printf("*V128\n");
	} else {
		// Invalid
		im_hw = 128;
		sd_close_file(&f);
		return false;
	}
	
	for (int pageIndex = 0; pageIndex < 3; pageIndex++){
		memset(sd_buffer, 0, sizeof(sd_buffer));
		bytesToRead = ZX_RAM_PAGE_SIZE;
		fd = sd_read_file(&f,sd_buffer,bytesToRead,&bytesRead);	
		if (fd != FR_OK){sd_close_file(&f);return false;}

		#ifdef DUMP_BUFFER
		printf("-------------[buff_dump]-------------\n");
		ptr=0;
		do{
			printf("BF--[%04X]",ptr);
			for (uint8_t col=0;col<16;col++){
				printf("     %02X",sd_buffer[ptr]);
				ptr++;
			}
			printf("\n");
		} while(ptr<sizeof(sd_buffer));
		printf("-------------[buff_dump]-------------\n");
		#endif

		switch (pageIndex) {
			case 0:
			printf("Page5\n");
			memcpy(&RAM[5*ZX_RAM_PAGE_SIZE],&sd_buffer,ZX_RAM_PAGE_SIZE);
			#ifdef DUMP_PAGES_V1
				printf("Page5 Used bytes[%04X]\n",usedBytes);
				ptr=0;
				do{
					printf("Page5[%04X]",5*ZX_RAM_PAGE_SIZE+ptr);
					for (uint8_t col=0;col<16;col++){
						printf("     %02X",RAM[5*ZX_RAM_PAGE_SIZE+ptr]);
						ptr++;
					}
					printf("\n");
				} while(ptr<ZX_RAM_PAGE_SIZE);
				printf("\n");
			#endif
			break;
			case 1:
			printf("Page2\n");
			memcpy(&RAM[2*ZX_RAM_PAGE_SIZE],&sd_buffer,ZX_RAM_PAGE_SIZE);
			#ifdef DUMP_PAGES_V1
				printf("Page 2 Used bytes[%04X]\n",usedBytes);
				ptr=0;
				do{
					printf("Page2[%04X]",2*ZX_RAM_PAGE_SIZE+ptr);
					for (uint8_t col=0;col<16;col++){
						printf("     %02X",RAM[2*ZX_RAM_PAGE_SIZE+ptr]);
						ptr++;
					}
					printf("\n");
				} while(ptr<ZX_RAM_PAGE_SIZE);
				printf("\n");
			#endif
			break;
			case 2:
			printf("Page0\n");
			memcpy(&RAM[zx_RAM_bank_active*ZX_RAM_PAGE_SIZE],&sd_buffer,ZX_RAM_PAGE_SIZE);
			#ifdef DUMP_PAGES_V1
				printf("Page%d Used bytes[%04X]\n",zx_RAM_bank_active,usedBytes);
				ptr=0;
				do{
					printf("Page%d[%04X]",zx_RAM_bank_active,zx_RAM_bank_active*ZX_RAM_PAGE_SIZE+ptr);
					for (uint8_t col=0;col<16;col++){
						printf("     %02X",RAM[zx_RAM_bank_active*ZX_RAM_PAGE_SIZE+ptr]);
						ptr++;
					}
					printf("\n");
				} while(ptr<ZX_RAM_PAGE_SIZE);
				printf("\n");
			#endif
			break;
		}
		
		#ifdef DUMP_BUFFER
			printf("-------------[buff_dump]-------------\n");
			ptr=0;
			do{
				printf("----[%04X]",ptr);
				for (uint8_t col=0;col<16;col++){
					printf("     %02X",sd_buffer[ptr]);
					ptr++;
				}
				printf("\n");
			} while(ptr<sizeof(sd_buffer));
			printf("-------------[buff_dump]-------------\n");
		#endif
	
	}
	if (im_hw==48){
		printf("48K\n");
		readCPUstateSNA(header,im_hw);
	    uint16_t SP = cpu.sp;
		//printf("SP_1:%04x\n",SP);
		cpu.pc=read_word_48k(SP);
        cpu.sp = SP + 2;
		//printf("PC:%04x,SP:%04x\n",cpu.pc,cpu.sp);
	} else if (im_hw==128) {
		printf("128K\n");
		bytesToRead=4;
		fd = sd_read_file(&f,buf,bytesToRead,&bytesRead);
		if (fd != FR_OK){sd_close_file(&f);return false;}
		//printf("bytesToRead=%d, bytesRead=%d\n",bytesToRead,bytesRead);            
		if (bytesRead != bytesToRead){sd_close_file(&f);return false;}

		last_out_7ffd = buf[2];
		readCPUstateSNA(header,im_hw);		
		cpu.pc=(buf[1] << 8) |buf[0]; // in 128K mode, recover stored PC
		//printf("PC:%04x,SP:%04x\n",cpu.pc,cpu.sp);
		/*
		49179	2		Содержимое регистра PC.
		49181	1		Содержимое порта 7FFDh.
		49182	1		1 если активно ПЗУ TR-DOS, 0 - если нет.
		49183	16384	Оставшиеся страницы памяти.
		*/
        // read remaining pages
        for (int page = 0; page < 8; page++) {
            if (page != zx_RAM_bank_active && page != 2 && page != 5) {
				//printf("Page%d\n",page);
				memset(sd_buffer, 0, sizeof(sd_buffer));
				bytesToRead = ZX_RAM_PAGE_SIZE;
				fd = sd_read_file(&f,sd_buffer,bytesToRead,&bytesRead);	
				if (fd != FR_OK){sd_close_file(&f);return false;}
				memcpy(&RAM[page*ZX_RAM_PAGE_SIZE],&sd_buffer,ZX_RAM_PAGE_SIZE);
            }
        }		
	}
	
	sd_close_file(&f);
	return true;

}// Конец процедуры

bool LoadScreenFromSNASnapshot(char *file_name){
	FIL f;
	int res;
	size_t bytesRead;
	UINT bytesToRead;	
	int fd=0;
	uint16_t ptr=0;
	uint16_t usedBytes=0;
	uint8_t im_hw = 0;			  //Тип оборудования для V2 V3
	char fileinfo[30];
	char sound[8];

	
	printf("load_screen_sna\n");
	
	memset(header_buf, 0, sizeof(header_buf));
	
	fd = sd_open_file(&f,file_name,FA_READ);
	//printf("sd_open_file=%d\n",fd);
	if (fd!=FR_OK){sd_close_file(&f);return false;}
	
	
	if (sd_file_size(&f)<SNA_48K_SIZE){
		sd_close_file(&f);return false;
	}
	if (sd_file_size(&f)>SNA_128K_SIZE2){
		sd_close_file(&f);return false;
	} 
	
	//printf("Begin read\n");
	bytesToRead = 27;
	
	fd = sd_read_file(&f,header_buf,bytesToRead,&bytesRead);
	//printf("bytesToRead=%d, bytesRead=%d\n",bytesToRead,bytesRead);
	if (fd!=FR_OK){sd_close_file(&f);return false;}
	if (bytesRead != bytesToRead){sd_close_file(&f);return false;}
	
	FileHeader* header = (FileHeader*)header_buf;
	printf("Header assign\n");
	
	
	
	//printf("Header PC=%04X\n",header->PC);
	//printf("Header A=%02X\n",header->A);
	memset(sound, 0, sizeof(sound));
	
	if (sd_file_size(&f) == SNA_48K_SIZE){
		// version 1
		im_hw = 48;
		zx_RAM_bank_active = 0;
		strncpy(sound, "Buzzer", 6);
		//readCPUstate(header);
		printf("*V48\n");
	} else if (sd_file_size(&f) > SNA_48K_SIZE){
		im_hw = 128;
		zx_RAM_bank_active = 0;
		strncpy(sound, "AY8910", 6);
		printf("*V128\n");
	} else {
		// Invalid
		im_hw = 128;
		sd_close_file(&f);
		return false;
	}
	
	memset(sd_buffer, 0, sizeof(sd_buffer));
	bytesToRead = 0x1B00;

	fd = sd_read_file(&f,sd_buffer,bytesToRead,&bytesRead);
	if (fd != FR_OK){sd_close_file(&f);return false;}

	ShowScreenshot(sd_buffer);
	memset(fileinfo, 0, sizeof(fileinfo));
	sprintf(fileinfo,"Type:.sna");
	draw_text_len(18+FONT_W*14,208, fileinfo,COLOR_TEXT,COLOR_BACKGOUND,22);
	memset(fileinfo, 0, sizeof(fileinfo));
	sprintf(fileinfo,"Mem:%dk Snd:%s",im_hw,sound);
	draw_text_len(18+FONT_W*14,216, fileinfo,COLOR_TEXT,COLOR_BACKGOUND,22);
	memset(fileinfo, 0, sizeof(fileinfo));
	strncpy(file_name,"A:/",3);
	strncpy(fileinfo,file_name,22);
	draw_text_len(18+FONT_W*14,224, fileinfo,COLOR_TEXT,COLOR_BACKGOUND,22);
	sd_close_file(&f);
	return true;	
}
