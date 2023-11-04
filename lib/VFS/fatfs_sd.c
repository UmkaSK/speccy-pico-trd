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
#include "VFS.h"
#ifdef USE_FATFS
#include "VFS_FATFS.h"

#define SD_DBG(x)
//printf("    [SD] %s\n", x)

#define SD_PRINT_ERROR()
//printf("[ERROR] %s() at line %d\n", __func__, __LINE__)

static volatile DSTATUS FATFS_SD_Stat = STA_NOINIT;

static BYTE FATFS_SD_CardType;

static inline uint8_t sd_detect(void)
{
#if FATFS_DETECT_PIN < 0
	return 1;
#else
	return gpio_get(FATFS_DETECT_PIN);		 /* read detect pin */
#endif
}

static inline uint8_t sd_enabled(void)
{
	return 1;
#if FATFS_WRITEPROTECT_PIN < 0
	return 1;
#else
	return gpio_get(FATFS_WRITEPROTECT_PIN); /* read write enable pin */
#endif
}

static uint8_t spi_send(spi_inst_t *SPIx, uint8_t data)
{
	uint8_t dst;
	spi_write_read_blocking(FATFS_SPI, (const uint8_t *)&data, &dst, 1);
	return dst;
}

static void init_spi(void)
{
	gpio_set_function(FATFS_SPI_MISO, GPIO_FUNC_SPI);
	gpio_set_function(FATFS_SPI_MOSI, GPIO_FUNC_SPI);
	gpio_set_function(FATFS_SPI_SCK, GPIO_FUNC_SPI);
	gpio_pull_up(FATFS_SPI_MISO);
	spi_set_format(FATFS_SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
	spi_init(FATFS_SPI, FATFS_SPI_BRG  );
}

/* Receive multiple byte */
static void rcvr_spi_multi(
	BYTE *buff, /* Pointer to data buffer */
	UINT btr	/* Number of bytes to receive (even number) */
)
{
	/* Read multiple bytes, send 0xFF as dummy */
	spi_read_blocking(FATFS_SPI, 0xFF, buff, btr);
}

#if FATFS_USE_WRITE
/* Send multiple byte */
static void xmit_spi_multi(
	const BYTE *buff, /* Pointer to the data */
	UINT btx		  /* Number of bytes to send (even number) */
)
{
	/* Write multiple bytes */
	spi_write_blocking(FATFS_SPI, (const uint8_t *)buff, btx);
}
#endif

/*-----------------------------------------------------------------------*/
/* Wait for card ready                                                   */
/*-----------------------------------------------------------------------*/
/* 1:Ready, 0:Timeout */
static int wait_ready(
	UINT wt /* Timeout [ms] */
)
{
	BYTE d;
	absolute_time_t timeout_time = make_timeout_time_us(wt * 1000);
	do
	{
		d = spi_send(FATFS_SPI, 0xFF);
	} while (d != 0xFF && 0 < absolute_time_diff_us(get_absolute_time(), timeout_time));

	if (d != 0xFF)
	{
		SD_PRINT_ERROR();
	}

	return (d == 0xFF) ? 1 : 0;
}

/*-----------------------------------------------------------------------*/
/* Deselect card and release SPI                                         */
/*-----------------------------------------------------------------------*/
static void deselect_card(void)
{
	FATFS_CS_HIGH;			   /* CS = H */
	spi_send(FATFS_SPI, 0xFF); /* Dummy clock (force DO hi-z for multiple slave SPI) */
}

/*-----------------------------------------------------------------------*/
/* Select card and wait for ready                                        */
/*-----------------------------------------------------------------------*/
/* 1:OK, 0:Timeout */
static int select_card(void)
{
	FATFS_CS_LOW;
	spi_send(FATFS_SPI, 0xFF); /* Dummy clock (force DO enabled) */
	if (wait_ready(500))
	{
		return 1; /* OK */
	}
	SD_PRINT_ERROR();
	deselect_card();
	return 0; /* Timeout */
}

/*-----------------------------------------------------------------------*/
/* Receive a data packet from the MMC                                    */
/*-----------------------------------------------------------------------*/
/* 1:OK, 0:Error */
static int rcvr_datablock(
	BYTE *buff, /* Data buffer */
	UINT btr	/* Data block length (byte) */
)
{
	BYTE token;
	absolute_time_t timeout_time = make_timeout_time_us(200 * 1000);
	do
	{
		// Wait for DataStart token in timeout of 200ms
		token = spi_send(FATFS_SPI, 0xFF);
		// This loop will take a time. Insert rot_rdq() here for multitask envilonment.
	} while ((token == 0xFF) && 0 < absolute_time_diff_us(get_absolute_time(), timeout_time));

	if (token != 0xFE)
	{
		SD_PRINT_ERROR();
		return 0; // Function fails if invalid DataStart token or timeout
	}

	rcvr_spi_multi(buff, btr); // Store trailing data to the buffer
	spi_send(FATFS_SPI, 0xFF);
	spi_send(FATFS_SPI, 0xFF); // Discard CRC

	return 1; // Function succeeded
}

/*-----------------------------------------------------------------------*/
/* Send a data packet to the MMC                                         */
/*-----------------------------------------------------------------------*/

#if FATFS_USE_IOCTL
/* 1:OK, 0:Failed */
static int xmit_datablock(
	const BYTE *buff, /* Ponter to 512 byte data to be sent */
	BYTE token		  /* Token */
)
{
	BYTE resp;
	if (!wait_ready(500))
	{
		SD_PRINT_ERROR();
		return 0; /* Wait for card ready */
	}

	spi_send(FATFS_SPI, token); /* Send token */
	if (token != 0xFD)
	{							   /* Send data if token is other than StopTran */
		xmit_spi_multi(buff, 512); /* Data */
		spi_send(FATFS_SPI, 0xFF);
		spi_send(FATFS_SPI, 0xFF); /* Dummy CRC */

		resp = spi_send(FATFS_SPI, 0xFF); /* Receive data resp */
		if ((resp & 0x1F) != 0x05)		  /* Function fails if the data packet was not accepted */
		{
			SD_PRINT_ERROR();
			return 0;
		}
	}
	return 1;
}
#endif

/*-----------------------------------------------------------------------*/
/* Send a command packet to the MMC                                      */
/*-----------------------------------------------------------------------*/
/* Return value: R1 resp (bit7==1:Failed to send) */
static BYTE send_cmd(
	BYTE cmd, /* Command index */
	DWORD arg /* Argument */
)
{
	BYTE n, res;
	if (cmd & 0x80)
	{ /* Send a CMD55 prior to ACMD<n> */
		cmd &= 0x7F;
		res = send_cmd(CMD55, 0);
		if (res > 1)
		{
			SD_PRINT_ERROR();
			return res;
		}
	}

	/* Select the card and wait for ready except to stop multiple block read */
	if (cmd != CMD12)
	{
		deselect_card();
		if (!select_card())
		{
			SD_PRINT_ERROR();
			return 0xFF;
		}
	}

	/* Send command packet */
	spi_send(FATFS_SPI, 0x40 | cmd);		/* Start + command index */
	spi_send(FATFS_SPI, (BYTE)(arg >> 24)); /* Argument[31..24] */
	spi_send(FATFS_SPI, (BYTE)(arg >> 16)); /* Argument[23..16] */
	spi_send(FATFS_SPI, (BYTE)(arg >> 8));	/* Argument[15..8] */
	spi_send(FATFS_SPI, (BYTE)arg);			/* Argument[7..0] */
	n = 0x01;								/* Dummy CRC + Stop */
	if (cmd == CMD0)
		n = 0x95; /* Valid CRC for CMD0(0) */
	if (cmd == CMD8)
		n = 0x87; /* Valid CRC for CMD8(0x1AA) */
	spi_send(FATFS_SPI, n);

	/* Receive command resp */
	if (cmd == CMD12)
	{
		spi_send(FATFS_SPI, 0xFF); /* Diacard following one byte when CMD12 */
	}

	n = 10; /* Wait for response (10 bytes max) */
	do
	{
		res = spi_send(FATFS_SPI, 0xFF);
	} while ((res & 0x80) && --n);

	return res; /* Return received response */
}

DSTATUS disk_initialize(BYTE pdrv)
{
	//SD_DBG(__func__);

	int res;
	BYTE n, cmd, ty, ocr[4];

#if FATFS_CS_PIN < 0
#warning sd card cs pin is not selected
#else
	gpio_set_function(FATFS_CS_PIN, GPIO_FUNC_XIP);
	gpio_init(FATFS_CS_PIN);
	gpio_set_dir(FATFS_CS_PIN, GPIO_OUT);
	gpio_put(FATFS_CS_PIN, 1);
#endif

#if FATFS_DETECT_PIN < 0
#else
	gpio_set_function(FATFS_DETECT_PIN, GPIO_FUNC_XIP);
	gpio_init(FATFS_DETECT_PIN);
#endif

#if FATFS_WRITEPROTECT_PIN < 0
#else
	gpio_set_function(FATFS_WRITEPROTECT_PIN, GPIO_FUNC_XIP);
	gpio_init(FATFS_WRITEPROTECT_PIN);
#endif

	init_spi();

	if (!sd_detect())
	{
		return STA_NODISK;
	}

	for (n = 10; n; n--)
		spi_send(FATFS_SPI, 0xFF);

	ty = 0;
	if (send_cmd(CMD0, 0) == 1)
	{
		/* Put the card SPI/Idle state */
		absolute_time_t timeout_time = make_timeout_time_us(1000 * 1000); /* Initialization timeout = 1 sec */
		if (send_cmd(CMD8, 0x1AA) == 1)
		{
			/* SDv2? */
			for (n = 0; n < 4; n++)
			{
				ocr[n] = spi_send(FATFS_SPI, 0xFF); /* Get 32 bit return value of R7 resp */
			}

			if (ocr[2] == 0x01 && ocr[3] == 0xAA) // 00, 00, 01, AA
			{
				/* Is the card supports vcc of 2.7-3.6V? */
				timeout_time = make_timeout_time_ms(1000 * 1000); /* Wait for end of initialization with ACMD41(HCS) */
				while ((res = send_cmd(ACMD41, 1UL << 30)) && 0 < absolute_time_diff_us(get_absolute_time(), timeout_time))
					;

				// Get the card capacity CCS: CMD58
				if ((res = send_cmd(CMD58, 0)) == 0)
				{ /* Check CCS bit in the OCR */
					for (n = 0; n < 4; n++)
					{
						ocr[n] = spi_send(FATFS_SPI, 0xFF);
					}
					ty = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2; /* Card id SDv2 */
				}
			}
		}
		else
		{
			/* Not SDv2 card */
			if (send_cmd(ACMD41, 0) <= 1)
			{
				/* SDv1 or MMC? */
				ty = CT_SD1;
				cmd = ACMD41; /* SDv1 (ACMD41(0)) */
			}
			else
			{
				ty = CT_MMC;
				cmd = CMD1; /* MMCv3 (CMD1(0)) */
			}
			while (0 < absolute_time_diff_us(get_absolute_time(), timeout_time) && send_cmd(cmd, 0))
				; /* Wait for end of initialization */

			if (0 < absolute_time_diff_us(get_absolute_time(), timeout_time) || send_cmd(CMD16, 512) != 0)
			{
				/* Set block length: 512 */
				ty = 0;
			}
		}
	}
	FATFS_SD_CardType = ty; /* Card type */
	deselect_card();

	if (ty)
	{								  /* OK */
		FATFS_SD_Stat &= ~STA_NOINIT; /* Clear STA_NOINIT flag */
	}
	else
	{
		/* Failed */
		FATFS_SD_Stat = STA_NOINIT;
	}

	if (!sd_enabled())
	{
		FATFS_SD_Stat |= STA_PROTECT;
	}
	else
	{
		FATFS_SD_Stat &= ~STA_PROTECT;
	}

	return FATFS_SD_Stat;
}

/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(BYTE pdrv)
{
	//SD_DBG(__func__);
	
	/* Check card detect pin if enabled */
	if (!sd_detect())
	{
		return STA_NOINIT;
	}

	/* Check if write is enabled */
	if (!sd_enabled())
	{
		FATFS_SD_Stat |= STA_PROTECT;
	}
	else
	{
		FATFS_SD_Stat &= ~STA_PROTECT;
	}

	return FATFS_SD_Stat; /* Return disk status */
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(
	BYTE pdrv,
	BYTE *buff,	  /* Data buffer to store read data */
	DWORD sector, /* Sector address (LBA) */
	UINT count	  /* Number of sectors to read (1..128) */
)
{
	//SD_DBG(__func__);

	if (!sd_detect() || (FATFS_SD_Stat & STA_NOINIT))
	{
		SD_PRINT_ERROR();
		return RES_NOTRDY;
	}

	if (!(FATFS_SD_CardType & CT_BLOCK))
	{
		sector *= 512; /* LBA ot BA conversion (byte addressing cards) */
	}

	if (count == 1)
	{ /* Single sector read */
		if ((send_cmd(CMD17, sector) == 0) && rcvr_datablock(buff, 512))
		{
			count = 0;
		}
		else
		{
			SD_PRINT_ERROR();
		}
	}
	else
	{
		/* Multiple sector read */
		if (send_cmd(CMD18, sector) == 0)
		{
			/* READ_MULTIPLE_BLOCK */
			do
			{
				if (!rcvr_datablock(buff, 512))
				{
					SD_PRINT_ERROR();
					break;
				}
				buff += 512;
			} while (--count);
			send_cmd(CMD12, 0); /* STOP_TRANSMISSION */
		}
	}
	deselect_card();

	return count ? RES_ERROR : RES_OK; /* Return result */
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FATFS_USE_WRITE
DRESULT disk_write(
	BYTE pdrv,
	const BYTE *buff, /* Data to be written */
	DWORD sector,	  /* Sector address (LBA) */
	UINT count		  /* Number of sectors to write (1..128) */
)
{
	//SD_DBG(__func__);

	if (!sd_detect())
	{
		SD_PRINT_ERROR();
		return RES_ERROR;
	}

	if (!sd_enabled())
	{
		SD_PRINT_ERROR(); // Write protected
		return RES_WRPRT;
	}

	if (FATFS_SD_Stat & STA_NOINIT)
	{
		SD_PRINT_ERROR();
		return RES_NOTRDY; /* Check drive status */
	}

	if (FATFS_SD_Stat & STA_PROTECT)
	{
		SD_PRINT_ERROR();
		return RES_WRPRT; /* Check write protect */
	}

	if (!(FATFS_SD_CardType & CT_BLOCK))
	{
		sector *= 512; /* LBA ==> BA conversion (byte addressing cards) */
	}

	if (count == 1)
	{
		/* Single sector write */
		if ((send_cmd(CMD24, sector) == 0) && xmit_datablock(buff, 0xFE))
		{
			count = 0;
		}
		else
		{
			SD_PRINT_ERROR();
		}
	}
	else
	{
		/* Multiple sector write */
		if (FATFS_SD_CardType & CT_SDC)
			send_cmd(ACMD23, count); /* Predefine number of sectors */
		if (send_cmd(CMD25, sector) == 0)
		{
			/* WRITE_MULTIPLE_BLOCK */
			do
			{
				if (!xmit_datablock(buff, 0xFC))
				{
					SD_PRINT_ERROR();
					break;
				}
				buff += 512;
			} while (--count);
			if (!xmit_datablock(0, 0xFD))
			{
				/* STOP_TRAN token */
				count = 1;
			}
		}
	}

	deselect_card();

	return count ? RES_ERROR : RES_OK; /* Return result */
}
#endif

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

#if FATFS_USE_IOCTL
DRESULT disk_ioctl(
	BYTE pdrv,
	BYTE cmd,  /* Control code */
	void *buff /* Buffer to send/receive control data */
)
{
	//SD_DBG(__func__);

	DRESULT res;
	BYTE n, csd[16];
	DWORD *dp, st, ed, csize;

	if (FATFS_SD_Stat & STA_NOINIT)
	{
		SD_PRINT_ERROR();
		return RES_NOTRDY; /* Check if drive is ready */
	}

	if (!sd_detect())
	{
		SD_PRINT_ERROR();
		return RES_NOTRDY;
	}

	res = RES_ERROR;

	switch (cmd)
	{
	case CTRL_SYNC: /* Wait for end of internal write process of the drive */
		if (select_card())
			res = RES_OK;
		break;

	case GET_SECTOR_COUNT: /* Get drive capacity in unit of sector (DWORD) */
		if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16))
		{
			if ((csd[0] >> 6) == 1)
			{ /* SDC ver 2.00 */
				csize = csd[9] + ((WORD)csd[8] << 8) + ((DWORD)(csd[7] & 63) << 16) + 1;
				*(DWORD *)buff = csize << 10;
			}
			else
			{ /* SDC ver 1.XX or MMC ver 3 */
				n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
				csize = (csd[8] >> 6) + ((WORD)csd[7] << 2) + ((WORD)(csd[6] & 3) << 10) + 1;
				*(DWORD *)buff = csize << (n - 9);
			}
			res = RES_OK;
		}
		break;

	case GET_BLOCK_SIZE: /* Get erase block size in unit of sector (DWORD) */
		if (FATFS_SD_CardType & CT_SD2)
		{
			/* SDC ver 2.00 */
			if (send_cmd(ACMD13, 0) == 0)
			{
				/* Read SD status */
				spi_send(FATFS_SPI, 0xFF);
				if (rcvr_datablock(csd, 16))
				{ /* Read partial block */
					for (n = 64 - 16; n; n--)
						spi_send(FATFS_SPI, 0xFF); /* Purge trailing data */
					*(DWORD *)buff = 16UL << (csd[10] >> 4);
					res = RES_OK;
				}
			}
		}
		else
		{
			/* SDC ver 1.XX or MMC */
			if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16))
			{
				/* Read CSD */
				if (FATFS_SD_CardType & CT_SD1)
				{
					/* SDC ver 1.XX */
					*(DWORD *)buff = (((csd[10] & 63) << 1) + ((WORD)(csd[11] & 128) >> 7) + 1) << ((csd[13] >> 6) - 1);
				}
				else
				{
					/* MMC */
					*(DWORD *)buff = ((WORD)((csd[10] & 124) >> 2) + 1) * (((csd[11] & 3) << 3) + ((csd[11] & 224) >> 5) + 1);
				}
				res = RES_OK;
			}
		}
		break;

	case CTRL_ERASE_SECTOR: /* Erase a block of sectors (used when _USE_ERASE == 1) */
		if (!(FATFS_SD_CardType & CT_SDC))
			break; /* Check if the card is SDC */
		if (disk_ioctl(pdrv, MMC_GET_CSD, csd))
			break; /* Get CSD */
		if (!(csd[0] >> 6) && !(csd[10] & 0x40))
			break; /* Check if sector erase can be applied to the card */
		dp = buff;
		st = dp[0];
		ed = dp[1]; /* Load sector block */
		if (!(FATFS_SD_CardType & CT_BLOCK))
		{
			st *= 512;
			ed *= 512;
		}
		/* Erase sector block */
		if (send_cmd(CMD32, st) == 0 && send_cmd(CMD33, ed) == 0 && send_cmd(CMD38, 0) == 0 && wait_ready(30000))
			res = RES_OK; /* FatFs does not check result of this command */
		break;

	default:
		SD_PRINT_ERROR();
		res = RES_PARERR;
	}

	deselect_card();

	return res;
}
#endif

//Use custom get_fattime function
//Implement RTC get time here if you need it
DWORD get_fattime(void)
{
	//SD_DBG(__func__);

	return ((DWORD)(2021 - 1980) << 25) // Year 2014
		   | ((DWORD)7 << 21)			// Month 7
		   | ((DWORD)10 << 16)			// Mday 10
		   | ((DWORD)16 << 11)			// Hour 16
		   | ((DWORD)0 << 5)			// Min 0
		   | ((DWORD)0 >> 1);			// Sec 0
}

#endif // USE_FATFS