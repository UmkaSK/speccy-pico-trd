//////////////////////////////////////////////////////////////////////////////////
//
//  Autors:
//		STM32 Team
//		Tilen Majerle 2015
//	http://stm32f4-discovery.net/2014/07/library-21-read-sd-card-fatfs-stm32f4xx-devices/
//
//  Raspberry Pi Pico Mod:
//		Georgi Angelov 2021
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//////////////////////////////////////////////////////////////////////////////////

#ifndef _VFS_FATFS_H_
#define _VFS_FATFS_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include "VFS.h"
#ifdef USE_FATFS

#include <ff.h>
#include <diskio.h>

#ifndef FATFS_LETTER
#define FATFS_LETTER "0:"
#endif

#ifndef FATFS_USE_WRITE
#define FATFS_USE_WRITE 1 /* 1: Enable disk_write function */
#endif

#ifndef FATFS_USE_IOCTL
#define FATFS_USE_IOCTL 1 /* 1: Enable disk_ioctl fucntion */
#endif

#ifndef FATFS_SPI
#define FATFS_SPI spi0
#endif

#ifndef FATFS_SPI_SCK
#define FATFS_SPI_SCK 9 /* SPI0_SCK */
#endif

#ifndef FATFS_SPI_MOSI
#define FATFS_SPI_MOSI 10 /* SPI0_TX */
#endif

#ifndef FATFS_SPI_MISO
#define FATFS_SPI_MISO 11 /* SPI0_RX */
#endif

#ifndef FATFS_CS_PIN
#define FATFS_CS_PIN 12 /* SPI0_CSn */
#endif

//TODO
#define FATFS_DETECT_PIN -1
//TODO
#define FATFS_WRITEPROTECT_PIN -1

#define FATFS_CS_LOW gpio_put(FATFS_CS_PIN, 0)
#define FATFS_CS_HIGH gpio_put(FATFS_CS_PIN, 1)

///////////////////////////////////////////////////////////////////

/* MMC/SD command */
#define CMD0 (0)           /* GO_IDLE_STATE */
#define CMD1 (1)           /* SEND_OP_COND (MMC) */
#define ACMD41 (0x80 + 41) /* SEND_OP_COND (SDC) */
#define CMD8 (8)           /* SEND_IF_COND */
#define CMD9 (9)           /* SEND_CSD */
#define CMD10 (10)         /* SEND_CID */
#define CMD12 (12)         /* STOP_TRANSMISSION */
#define ACMD13 (0x80 + 13) /* SD_STATUS (SDC) */
#define CMD16 (16)         /* SET_BLOCKLEN */
#define CMD17 (17)         /* READ_SINGLE_BLOCK */
#define CMD18 (18)         /* READ_MULTIPLE_BLOCK */
#define CMD23 (23)         /* SET_BLOCK_COUNT (MMC) */
#define ACMD23 (0x80 + 23) /* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24 (24)         /* WRITE_BLOCK */
#define CMD25 (25)         /* WRITE_MULTIPLE_BLOCK */
#define CMD32 (32)         /* ERASE_ER_BLK_START */
#define CMD33 (33)         /* ERASE_ER_BLK_END */
#define CMD38 (38)         /* ERASE */
#define CMD55 (55)         /* APP_CMD */
#define CMD58 (58)         /* READ_OCR */

#endif // USE_FATFS

#ifdef __cplusplus
}
#endif
#endif // _VFS_FATFS_H_
