//---------------------------------------------------------------------------------
#include "util_z80.h"
#include "util_sd.h"
#include <VFS.h>
#include <fcntl.h>
#include <string.h>
#include <zx_emu/z80.h>
#include "zx_emu/aySoundSoft.h"
#include "zx_emu/zx_machine.h"
#include "screen_util.h"
#include "trdos.h"

extern char sd_buffer[SD_BUFFER_SIZE];
extern int last_error;
extern uint8_t RAM[0x4000*8]; 
//------------------------------------------

//------------------------------------------
bool ReadCatalog(char *file_name, bool open_file)
{
	FIL f;
	size_t bytesRead;
	int fd = 0;
	//	char fileinfo[30];
	UINT bytesToRead;

	//	memset(bufferOut, 0, 0x0900);

	// размер каталога диcка trd  8*256 + 256 9 сектор 0 дорожки 2304 0x900
	fd = sd_open_file(&f, file_name, FA_READ); // открыть файл file_name на чтение
											   // printf("sd_open_file=%d\n",fd);
	//  printf("TRD: %s\n",file_name);

	if (fd != FR_OK)
	{
		sd_close_file(&f);
		return false;
	} // закрыть файл ошибка

	fd = sd_read_file(&f, sd_buffer, 0x900, &bytesRead); // прочитать 9 секторов в bufferIn
	if (fd != FR_OK)
	{
		sd_close_file(&f);
		return false;
	} // ошибка
	printf("bytesRead=%d\n", bytesRead);
	if (bytesRead != 0x900)
	{
		sd_close_file(&f);
		return false;
	} // ошибка
	  // memcpy(bufferOut,bufferIn,bytesRead);

	// всего может быть 2048/16= 128 файлов
#define POS_X 24 + FONT_W * 14
#define POS_Y 32

	char symb[8];
	int x = POS_X;
	int y = POS_Y;
	int j = 0;
	// int i = 44;// количество выводимых названий файлов
	int i = sd_buffer[0x8e4]; // Количество файлов на диске, включая удаленные.
	if (i > 44)///////////////////////////////////////
		i = 44; // количество выводимых названий файлов

	draw_rect(129, 16, 182, 24 * 8, COLOR_BACKGOUND, true); // clear

	while (i != 0)
	{

		//  if (bufferOut[j] ==0) {i--; j = j+16; continue;}

		symb[0] = sd_buffer[j];
		symb[1] = sd_buffer[j + 1];
		symb[2] = sd_buffer[j + 2];
		symb[3] = sd_buffer[j + 3];
		symb[4] = sd_buffer[j + 4];
		symb[5] = sd_buffer[j + 5];
		symb[6] = sd_buffer[j + 6];
		symb[7] = sd_buffer[j + 7];

		if (sd_buffer[j + 8] == 'B')
		{
			if (sd_buffer[j + 0] == 0x01)
				break;
			{

				draw_text_len(x, y, symb, CL_GRAY, COLOR_BACKGOUND, 8);
				y = y + 8;
			}
		}
		if (y >= POS_Y + 22 * 8)
		{
			x = POS_X + 88;
			y = POS_Y;
		}

		i--;
		j = j + 16;
	}
	// sprintf(fileinfo,"Type:.TRD - TR-DOS");
	//  системная информация 0x800 начало
	//  имя диска +245 0x8f5
	symb[0] = sd_buffer[0x8f5];
	symb[1] = sd_buffer[0x8f5 + 1];
	symb[2] = sd_buffer[0x8f5 + 2];
	symb[3] = sd_buffer[0x8f5 + 3];
	symb[4] = sd_buffer[0x8f5 + 4];
	symb[5] = sd_buffer[0x8f5 + 5];
	symb[6] = sd_buffer[0x8f5 + 6];
	symb[7] = sd_buffer[0x8f5 + 7];
	draw_text_len(24 + FONT_W * 14, 16, symb, CL_CYAN, COLOR_BACKGOUND, 8);

	sd_close_file(&f);
	return true;
}
/* for (int x = 0; x < 2; x++)
	{
		for (int i = 0; i < 23; i++)
		{
			//  if (bufferOut[j-8] !='B') {i--; j = j+16; continue;}
			//  if (bufferOut[j] ==0) {i--; j = j+16; continue;}

			symb[0] = sd_buffer[j];
			symb[1] = sd_buffer[j + 1];
			symb[2] = sd_buffer[j + 2];
			symb[3] = sd_buffer[j + 3];
			symb[4] = sd_buffer[j + 4];
			symb[5] = sd_buffer[j + 5];
			symb[6] = sd_buffer[j + 6];
			symb[7] = sd_buffer[j + 7];

			j = j + 16;

			draw_text_len(24 + x * 88 + FONT_W * 14, 32 + i * 8, symb, CL_GRAY, COLOR_BACKGOUND, 8);
		}
	} */