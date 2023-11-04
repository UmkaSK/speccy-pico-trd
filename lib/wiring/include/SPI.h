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

#ifndef __SPI_CLASS_H__
#define __SPI_CLASS_H__

#ifdef __cplusplus

#include "Common.h"
#include "Stream.h"
#include "RingBuffer.h"
#include <hardware/spi.h>
#include <hardware/gpio.h>

typedef enum
{
    SPI_MODE0 = 0, // CPOL : 0  | CPHA : 0
    SPI_MODE1,     // CPOL : 0  | CPHA : 1
    SPI_MODE2,     // CPOL : 1  | CPHA : 0
    SPI_MODE3      // CPOL : 1  | CPHA : 1
} SPIDataMode;

class SPISettings
{
public:
    SPISettings(uint32_t clockFrequency, uint8_t bitOrder, uint8_t dataMode)
    {
        clock = clockFrequency; // Hz
        order = bitOrder;
        mode = dataMode;
    }

    SPISettings()
    {
        clock = 1000000; // Hz
        order = MSBFIRST;
        mode = SPI_MODE0;
    }

private:
    uint32_t clock;
    uint8_t order;
    uint8_t mode;
    friend class SPIClass;
};

class SPIClass
{
private:
    spi_inst_t *spi;
    int _cs;
    uint32_t _brg_hz;

    uint32_t _clk_polarity;
    uint32_t _clk_format;
    uint32_t _bit_order;
    uint32_t _data_bits;
    uint32_t _mode;

    void init_default()
    {
        _mode = 0;             //
        _clk_polarity = 0;     // cpol
        _clk_format = 0;       // cpha
        _bit_order = MSBFIRST; // only 1
        _data_bits = 8;        // 4..16
        _brg_hz = 1000000;     // max 31 250 000
        _cs = -1;
    }

    inline uint8_t reverseByte(uint8_t b)
    {
        b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
        b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
        b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
        return b;
    }

public:
    SPIClass(uint8_t num)
    {
        num ? spi = spi1 : spi0;
        init_default();
    }

    SPIClass(spi_inst_t *inst)
    {
        spi = inst;
        init_default();
    }

    void setPins(int8_t sck, int8_t miso, int8_t mosi, int8_t ss = -1)
    {
        if (miso > -1)
            gpio_set_function(miso, GPIO_FUNC_SPI);
        if (mosi > -1)
            gpio_set_function(mosi, GPIO_FUNC_SPI);
        if (sck > -1)
            gpio_set_function(sck, GPIO_FUNC_SPI);
        if (ss > -1)
        {
            gpio_init(ss);
            gpio_set_dir(ss, GPIO_OUT);
            gpio_put(ss, 1);
            _cs = ss;
        }
    }

    void begin(int8_t sck = -1, int8_t miso = -1, int8_t mosi = -1, int8_t ss = -1)
    {
        setPins(sck, miso, mosi, ss);
        spi_init(spi, _brg_hz);
    }

    void end()
    {
        spi_deinit(spi);
    }

    uint8_t transfer(uint8_t tx)
    {
        uint8_t rx = 0;
        if (_cs > -1)
        {
            gpio_put(_cs, 0);
            //asm volatile("nop \n nop"); // NOP?
        }

        if (_bit_order == LSBFIRST)
            tx = reverseByte(tx);

        spi_write_read_blocking(spi, &tx, &rx, 1);

        if (_cs > -1)
            gpio_put(_cs, 1);

        return (_bit_order == LSBFIRST) ? reverseByte(rx) : rx;
    }

    uint16_t transfer(uint16_t data)
    {
        union
        {
            uint16_t val;
            struct
            {
                uint8_t lsb;
                uint8_t msb;
            };
        } t;
        t.val = data;
        if (_bit_order == LSBFIRST)
        {
            t.lsb = transfer(t.lsb);
            t.msb = transfer(t.msb);
        }
        else
        {
            t.msb = transfer(t.msb);
            t.lsb = transfer(t.lsb);
        }
        return t.val;
    }

    int transfer(uint8_t *buf, size_t count)
    {
        return spi_write_read_blocking(spi, buf, buf, count); // MSB
    }

    void write(uint8_t *buf, size_t count) { transfer(buf, count); }

    void write(uint16_t *buf, size_t count)
    {
        //LSBFIRST
        spi_set_format(spi, 16, (spi_cpol_t)_clk_polarity, (spi_cpha_t)_clk_format, (spi_order_t)_bit_order);
        spi_write16_blocking(spi, buf, count);
        spi_set_format(spi, _data_bits, (spi_cpol_t)_clk_polarity, (spi_cpha_t)_clk_format, (spi_order_t)_bit_order);
    }

    void setFrequency(uint32_t Hz)
    {
        if (_brg_hz != Hz)
        {
            _brg_hz = Hz;
            spi_set_baudrate(spi, Hz);
        }
    }

    void setDataMode(uint8_t mode)
    {
        if (_mode != mode)
        {
            _mode = mode;
            _clk_format = (bool)(mode & 1);
            _clk_polarity = (bool)(mode & 2);
        }
    }

    void setBitOrder(uint8_t order)
    {
        if (_bit_order != order)
        {
            _bit_order = order;
        }
    }

    void beginTransaction(SPISettings settings)
    {
        setFrequency(settings.clock);
        setDataMode(settings.mode);
        setBitOrder(settings.order);
        spi_set_format(spi, _data_bits, (spi_cpol_t)_clk_polarity, (spi_cpha_t)_clk_format, (spi_order_t)_bit_order);
    }

    void endTransaction(void) {}

    uint32_t getClockDivider() { return 0; }
    void setClockDivider(uint8_t div) {}
    void usingInterrupt(int interruptNumber) {}
    void notUsingInterrupt(int interruptNumber) {}
    void attachInterrupt(){};
    void detachInterrupt(){};
};

extern SPIClass SPI;

#endif // __cplusplus
#endif //__SPI_CLASS_H__