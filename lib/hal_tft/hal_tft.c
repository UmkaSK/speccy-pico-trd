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
 * 
   This library is helper for SPI(DMA) and TFT displays as ST77xx, ILI93xx
   
****************************************************************************************/

#include "hal_tft.h"

static int dma_channel_tx;
static dma_channel_config dma_cfg_tx;

void tft_spi_put(uint16_t data /* cmd/data/color */,
                 int bits /* 8 for cmd/data or 16 for color */)
{
    spi_set_format(TFT_SPI_FORMAT(bits));
    spi_write16_blocking(TFT_SPI, &data, 1);
}

void tft_spi_put_data(uint16_t data, /* cmd/data/color */
                      uint32_t size,
                      int bits /* 8 for cmd/data or 16 for color */)
{
    spi_set_format(TFT_SPI_FORMAT(bits));
    channel_config_set_transfer_data_size(&dma_cfg_tx, bits == 16 ? DMA_SIZE_16 : DMA_SIZE_8);
    channel_config_set_read_increment(&dma_cfg_tx, false);
    dma_channel_configure(dma_channel_tx, &dma_cfg_tx, &spi_get_hw(TFT_SPI)->dr, &data, size, true);
    dma_channel_wait_for_finish_blocking(dma_channel_tx);
    while (spi_get_hw(TFT_SPI)->sr & SPI_SSPSR_BSY_BITS) // wait to finish
        tight_loop_contents();
}

void tft_spi_put_buffer(uint16_t *buffer,
                        uint32_t size,
                        int bits /* 8 or 16 for draw images */)
{
    spi_set_format(TFT_SPI_FORMAT(bits));
    channel_config_set_transfer_data_size(&dma_cfg_tx, bits == 16 ? DMA_SIZE_16 : DMA_SIZE_8);
    channel_config_set_read_increment(&dma_cfg_tx, size > 1);
    dma_channel_configure(dma_channel_tx, &dma_cfg_tx, &spi_get_hw(TFT_SPI)->dr, buffer, size, true);
    dma_channel_wait_for_finish_blocking(dma_channel_tx);
    while (spi_get_hw(TFT_SPI)->sr & SPI_SSPSR_BSY_BITS) // wait to finish
        tight_loop_contents();
}

void tft_write_cmd(uint8_t c)
{
    TFT_DC_CMD();
    TFT_CS_ENABLE();
    tft_spi_put(c, 8);
    TFT_CS_DISABLE();
}

void tft_write_data(uint8_t d8)
{
    TFT_DC_DATA();
    TFT_CS_ENABLE();
    tft_spi_put(d8, 8);
    TFT_CS_DISABLE();
}

void tft_write_data16(uint16_t d16)
{
    TFT_DC_DATA();
    TFT_CS_ENABLE();
    tft_spi_put(d16, 16);
    TFT_CS_DISABLE();
}

/* tft init commands */
void tft_list_init(const uint8_t *address)
{
    uint8_t *addr = (uint8_t *)address;
    uint8_t numCommands, numArgs;
    uint16_t ms;
    numCommands = *addr++;
    while (numCommands--)
    {
        tft_write_cmd(*addr++);
        numArgs = *addr++;
        ms = numArgs & 0x80;
        numArgs &= 0x7F;
        while (numArgs--)
            tft_write_data(*addr++);
        if (ms)
        {
            ms = *addr++;
            if (ms == 255)
                ms = 500;
            TFT_DELAY_MS(ms);
        }
    }
}

/* init pins, spi, dma */
void tft_init(void)
{
    dma_channel_tx = dma_claim_unused_channel(true);
    dma_cfg_tx = dma_channel_get_default_config(dma_channel_tx);
    channel_config_set_dreq(&dma_cfg_tx, TFT_SPI == spi0 ? DREQ_SPI0_TX : DREQ_SPI1_TX);
    channel_config_set_write_increment(&dma_cfg_tx, false);

    if (TFT_DC > -1)
    {
        gpio_init(TFT_DC);
        gpio_set_dir(TFT_DC, GPIO_OUT);
    }

    if (TFT_CS > -1)
    {
        gpio_init(TFT_CS);
        gpio_set_dir(TFT_CS, GPIO_OUT);
    }

    if (TFT_MISO > -1)
        gpio_set_function(TFT_MISO, GPIO_FUNC_SPI);

    if (TFT_MOSI > -1)
        gpio_set_function(TFT_MOSI, GPIO_FUNC_SPI);

    if (TFT_SCLK > -1)
        gpio_set_function(TFT_SCLK, GPIO_FUNC_SPI);

    spi_init(TFT_SPI, TFT_SPI_BRG);
    spi_set_format(TFT_SPI_FORMAT(8));

    TFT_CS_ENABLE();
    if (TFT_RST > -1)
    {
        gpio_init(TFT_RST);
        gpio_set_dir(TFT_RST, GPIO_OUT);
        gpio_put(TFT_RST, 1);
        TFT_DELAY_MS(5);
        gpio_put(TFT_RST, 0);
        TFT_DELAY_MS(20);
        gpio_put(TFT_RST, 1);
        TFT_DELAY_MS(150);
    }
}
