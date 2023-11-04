/* 

WIKI:
        https://github.com/Wiz-IO/wizio-pico/wiki/ARDUINO#vfs--file-system

OTHER USER CONFIG KEYS

LFS:    https://github.com/littlefs-project/littlefs   

        https://github.com/Wiz-IO/framework-wizio-pico/blob/main/arduino/libraries/RP2040/VFS/VFS_LFS.h

FATFS:  http://elm-chan.org/fsw/ff/00index_e.html

        https://github.com/Wiz-IO/framework-wizio-pico/blob/main/arduino/libraries/RP2040/VFS/VFS_FATFS.h

*/

#define MAX_OPEN_FILES          4

#define USE_FATFS               /* Enable FatFS ... 0:/file_path */
#define FATFS_SPI               spi0
#define FATFS_SPI_BRG           10000000
#define FATFS_SPI_SCK           2 /* SPI1_SCK  */
#define FATFS_SPI_MOSI          3 /* SPI1_TX   */
#define FATFS_SPI_MISO          4 /* SPI1_RX   */
#define FATFS_CS_PIN            5 /* SPI1_CSn  */