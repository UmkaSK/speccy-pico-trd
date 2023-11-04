////////////////////////////////////////////////////////////////////////////////////////
//
//      2021 Georgi Angelov
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
////////////////////////////////////////////////////////////////////////////////////////

/****************************************************************************************
   This library is helper for SPI(DMA) and TFT displays as ST77xx, ILI93xx


    Need <hal_tft_config.h>
    Example for ST7789 240x240


#ifndef _HAL_TFT_CFG_H_
#define _HAL_TFT_CFG_H_
#ifdef __cplusplus
extern "C"
{
#endif

#define TFT_WIDTH   240
#define TFT_HEIGHT  240

#define TFT_MOSI    15
#define TFT_SCLK    14
#define TFT_MISO    -1 // not used
#define TFT_CS      -1 // not used
#define TFT_DC      12
#define TFT_RST     13

#define TFT_SPI spi1

#define TFT_SPI_BRG 31250000u // Hz, is max for pico default ... 32 fps

#define TFT_SPI_FORMAT(BITS) TFT_SPI, BITS, SPI_CPOL_1, SPI_CPHA_0, SPI_MSB_FIRST

#ifdef __cplusplus
}
#endif
#endif // _HAL_TFT_CFG_H_


****************************************************************************************/


#ifndef _HAL_TFT_H_
#define _HAL_TFT_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include <hardware/spi.h>
#include <hardware/gpio.h>
#include <hardware/dma.h>
#include <hal_tft_config.h>

#define TFT_CS_DISABLE()         \
    {                            \
        if (TFT_CS > -1)         \
            gpio_put(TFT_CS, 1); \
    }

#define TFT_CS_ENABLE()          \
    {                            \
        if (TFT_CS > -1)         \
            gpio_put(TFT_CS, 0); \
    }

#define TFT_DC_DATA()            \
    {                            \
        if (TFT_DC > -1)         \
            gpio_put(TFT_DC, 1); \
    }

#define TFT_DC_CMD()             \
    {                            \
        if (TFT_DC > -1)         \
            gpio_put(TFT_DC, 0); \
    }

#define TFT_DELAY_MS(T) sleep_ms(T)

    void tft_spi_put(uint16_t data, int bits);
    void tft_spi_put_data(uint16_t data, uint16_t size, int bits);
    void tft_spi_put_buffer(uint16_t *buffer, uint16_t size, int bits);

    void tft_init(void);
    void tft_list_init(const uint8_t *address);

    void tft_write_cmd(uint8_t c);
    void tft_write_data(uint8_t d8);
    void tft_write_data16(uint16_t d16);

#ifdef __cplusplus
}
#endif
#endif // _HAL_TFT_H_