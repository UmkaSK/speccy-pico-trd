#include "VGA.h"
#include "hardware/clocks.h"

#include "hardware/structs/pll.h"
#include "hardware/structs/systick.h"
#include "hardware/flash.h"

#include "hardware/dma.h"
#include "hardware/irq.h"
#include <string.h> 
#include "pico/mem_ops.h"
#include "hardware/pio.h"
#include "pico/stdlib.h"



//программа конвертации адреса

uint16_t pio_program1_instructions2[] = {             
               
    // 0x80a0, //  0: pull   block                      
    // 0xa027, //  1: mov    x, osr                     
    //         //     .wrap_target
    0x80a0, //  2: pull   block                      
    // 0x4061, //  3: in     null, 1                    
    0x40e8, //  4: in     osr, 8                     
    0x4037, //  5: in     x, 23                      
    0x8020, //  6: push   block                      
            //     .wrap


};


const struct pio_program pio_program_conv = {
    .instructions = pio_program1_instructions2,
    .length = 4,
    .origin = -1,
};
//программа вывода 8 бит на пины VGA
uint16_t pio_program_VGA_instructions[] = {
                       
        //     .wrap_target

               //     .wrap_target
    // 0x80a0, //  0: pull   block                      
    0x6008, //  1: out    pins, 8                    
            //     .wrap                
            //     .wrap
};

const struct pio_program pio_program_VGA = {
    .instructions = pio_program_VGA_instructions,
    .length = 1,
    .origin = -1,
};

//static uint16_t conv_color[512]; ///???????????????????
static uint32_t conv_color[512];
static uint16_t* conv_color_align;


#define VISIBLE_LINES (480)
#define FRAME_LINES  (525)
#define LINE_SIZE (400)

//массив управления потоком данных с видеобуфера на конвертор адреса
static uint32_t screen_CMD_dma[VISIBLE_LINES+2];
//управляющий массив основного канала
static uint32_t CMD_DMA0[FRAME_LINES*2];
static uint32_t CMD_DMA_TMPL[8][4];
static uint32_t RELOAD_DATA[2];
static uint32_t line_tmpl[LINE_SIZE/2];



#define SCREEN_W (320)
#define SCREEN_H (240)
extern uint8_t g_gbuf[];

//цвета спектрума в формате 6 бит
const uint8_t zx_color_6b[]={
        0b00000000,
        0b00000010,
        0b00100000,
        0b00100010,
        0b00001000,
        0b00001010,
        0b00101000,
        0b00101010,
        0b00000000,
        0b00000011,
        0b00110000,
        0b00110011,
        0b00001100,
        0b00001111,
        0b00111100,
        0b00111111
    };


static void pio_set_x(PIO pio, int sm, uint32_t v) {
   uint instr_shift = pio_encode_in(pio_x, 4);
   uint instr_mov = pio_encode_mov(pio_x, pio_isr);
  for (int i=0;i<8;i++)
  {
    const uint32_t nibble = (v >> (i * 4)) & 0xf;
    pio_sm_exec(pio, sm, pio_encode_set(pio_x, nibble));
    pio_sm_exec(pio, sm, instr_shift);
  }
  pio_sm_exec(pio, sm, instr_mov);
}


void startVGA()
{

    //параметры для видеосигнала 
    float pix_freq=(25175000.0/2);
    uint8_t sync_mask8=0b11000000;


    //настройка SM для конвертации
    uint offset0 = pio_add_program(PIO_VGA_conv, &pio_program_conv);
    uint sm_c=SM_VGA_conv;
    conv_color_align = ((uint32_t)conv_color&0xfffffe00)+0x200;
    pio_set_x(PIO_VGA_conv,sm_c,((uint32_t)conv_color_align>>9));


    pio_sm_config c_c = pio_get_default_sm_config();
    sm_config_set_wrap(&c_c, offset0 + 0, offset0 + (pio_program_conv.length-1));
    sm_config_set_in_shift(&c_c, true, false, 32);

   

    pio_sm_init(PIO_VGA_conv, sm_c, offset0, &c_c);
    pio_sm_set_enabled(PIO_VGA_conv, sm_c, true);

    //инициализация массива конвертации данных видеобуфера
    for(int ci=0;ci<256;ci++)
    {
        uint8_t cl=zx_color_6b[ci&0xf]|sync_mask8;
        uint8_t ch=zx_color_6b[(ci>>4)&0xf]|sync_mask8;
        conv_color_align[ci]=(ch<<8)|cl;     
    }


    // //test
        
        // PIO_VGA_conv->txf[sm_c]=0x01;
        // PIO_VGA_conv->txf[sm_c]=0x02;
        // PIO_VGA_conv->txf[sm_c]=0x04;
        // PIO_VGA_conv->txf[sm_c]=0x08;

    //     sleep_ms(2000);
    //     printf("addr  =0x%08x\n",conv_color);
    //     printf("addr  end =0x%08x\n",&conv_color[512]);

    //     printf("addr2 =0x%08x\n",conv_color_align);
    //     printf("addr2 end=0x%08x\n",&conv_color_align[256]);

        // for(int i=0;i<10;i++)
        // {
        //     sleep_ms(1);
        //     printf("0x%08x\n",PIO_VGA_conv->rxf[sm_c]);

        // };

    //     return;

//инициализация PIO для вывода VGA сигнала
    //загрузка программы в один из PIO
    uint offset = pio_add_program(PIO_VGA, &pio_program_VGA);
    uint sm=SM_VGA;

    for(int i=0;i<8;i++){
        gpio_init(beginVGA_PIN+i);
        gpio_set_dir(beginVGA_PIN+i,GPIO_OUT);
        pio_gpio_init(PIO_VGA, beginVGA_PIN+i);};
  
    pio_sm_set_consecutive_pindirs(PIO_VGA, sm, beginVGA_PIN, 8, true);//конфигурация пинов на выход
    //pio_sm_config c = pio_vga_program_get_default_config(offset); 

    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + 0, offset + (pio_program_VGA.length-1));


    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);//увеличение буфера TX за счёт RX до 8-ми 
    sm_config_set_out_shift(&c, true, true, 16);
    sm_config_set_out_pins(&c, beginVGA_PIN, 8);
    pio_sm_init(PIO_VGA, sm, offset, &c);

    pio_sm_set_enabled(PIO_VGA, sm, true);
    //пиксельклок для SM 
    float fdiv=clock_get_hz(clk_sys)/pix_freq;//частота VGA по умолчанию
    uint32_t div32=(uint32_t) (fdiv * (1 << 16)+0.5);
    PIO_VGA->sm[sm].clkdiv=div32&0xfffff000; //делитель для конкретной sm 

//канал для передачи даных экранного буфера  в конвертор и канал для его управления
    int dma_screen_conv=dma_claim_unused_channel(true);
    int dma_screen_conv_cmd=dma_claim_unused_channel(true);
    uint8_t* v_buf=g_gbuf;

    dma_channel_config c_conv = dma_channel_get_default_config(dma_screen_conv);
    channel_config_set_transfer_data_size(&c_conv, DMA_SIZE_8);

    channel_config_set_read_increment(&c_conv, true);
    channel_config_set_write_increment(&c_conv, false);

    uint dreq0=DREQ_PIO1_TX0+sm_c;
    if (PIO_VGA_conv==pio0) dreq0=DREQ_PIO0_TX0+sm_c;
    channel_config_set_dreq(&c_conv, dreq0);

    channel_config_set_chain_to(&c_conv, dma_screen_conv_cmd);                        // chain to other channel
   
    dma_channel_configure(
        dma_screen_conv,
        &c_conv,
        &PIO_VGA_conv->txf[sm_c], // Write address 
        v_buf,             // read address 
        SCREEN_W/2, //
        false             // Don't start yet
    );

    dma_channel_config c_conv_ctrl = dma_channel_get_default_config(dma_screen_conv_cmd);
    channel_config_set_transfer_data_size(&c_conv_ctrl, DMA_SIZE_32);

    channel_config_set_read_increment(&c_conv_ctrl, true);
    channel_config_set_write_increment(&c_conv_ctrl, false);

    channel_config_set_chain_to(&c_conv_ctrl, dma_screen_conv);                        // chain to other channel
   
    dma_channel_configure(
        dma_screen_conv_cmd,
        &c_conv_ctrl,
        &dma_hw->ch[dma_screen_conv].read_addr, // Write address 
        screen_CMD_dma,             // read address 
        1, //
        true             // start yet
    );
//данные для управления каналом DMA читающим видеобуфер
    for(int i=0;i<VISIBLE_LINES+2;i++) screen_CMD_dma[i]=&v_buf[((i>>1)%SCREEN_H)*SCREEN_W/2];
    screen_CMD_dma[0]=&v_buf[9];
    


   

// ДМА канал для данных    передающий изображение на VGA SM
    int dma_screen_conv_out=dma_claim_unused_channel(true);
    dma_channel_config c_pix = dma_channel_get_default_config(dma_screen_conv_out);
    channel_config_set_transfer_data_size(&c_pix, DMA_SIZE_16);

    channel_config_set_read_increment(&c_pix, false);
    channel_config_set_write_increment(&c_pix, false);

    // uint dreq1=DREQ_PIO1_TX0+sm;
    // if (PIO_VGA==pio0) dreq1=DREQ_PIO0_TX0+sm;
    // channel_config_set_dreq(&c_pix, dreq1);
   
    dma_channel_configure(
        dma_screen_conv_out,
        &c_pix,
        &PIO_VGA->txf[sm], // Write address 
        conv_color_align,             // read address 
        1, //
        false             // Don't start yet
    );
   
//

    int dma_chan_ctrl=dma_claim_unused_channel(true);
    int dma_chan_ctrl0=dma_claim_unused_channel(true);
    int dma_chan=dma_claim_unused_channel(true);
//основной ДМА канал для данных на VGA передающий синхру или адрес данных на изображение

    dma_channel_config c0 = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&c0, DMA_SIZE_16);

    channel_config_set_read_increment(&c0, true);
    channel_config_set_write_increment(&c0, false);

    uint dreq=DREQ_PIO1_TX0+sm;
    if (PIO_VGA==pio0) dreq=DREQ_PIO0_TX0+sm;

    channel_config_set_dreq(&c0, dreq);
    channel_config_set_chain_to(&c0, dma_chan_ctrl);                        // chain to other channel
   
    dma_channel_configure(
        dma_chan,
        &c0,
        &PIO_VGA->txf[sm], // Write address 
        conv_color_align,             // read address 
        100, //
        false             // Don't start yet
    );
    //канал DMA для контроля 1
    dma_channel_config c1 = dma_channel_get_default_config(dma_chan_ctrl);
    channel_config_set_transfer_data_size(&c1, DMA_SIZE_32);
    
    channel_config_set_read_increment(&c1, true);
    channel_config_set_write_increment(&c1, false);
    
    //  channel_config_set_chain_to(&c1, dma_chan);                         // chain to other channel
    //channel_config_set_dreq(&c1, DREQ_PIO0_TX0);

    //channel_config_set_ring(&c1,true,4);

    dma_channel_configure(
        dma_chan_ctrl,
        &c1,
        &dma_hw->ch[dma_chan_ctrl0].al3_read_addr_trig, // Write address 
        CMD_DMA0,             // read address
        1, // 
        false             // Don't start yet
    );

    //канал DMA для контроля 0
    dma_channel_config c2 = dma_channel_get_default_config(dma_chan_ctrl0);
    channel_config_set_transfer_data_size(&c2, DMA_SIZE_32);
    
    channel_config_set_read_increment(&c2, true);
    channel_config_set_write_increment(&c2, true);
    
    // channel_config_set_chain_to(&c1, dma_chan);                         // chain to other channel
    //channel_config_set_dreq(&c1, DREQ_PIO0_TX0);

    channel_config_set_ring(&c2,true,4);

    dma_channel_configure(
        dma_chan_ctrl0,
        &c2,
        &dma_hw->ch[dma_chan].read_addr, // Write address 
        CMD_DMA_TMPL,             // read address
        4, // 
        false             // Don't start yet
    );
//создание управляющих шаблонов и загрузка основной управляющей последовательности
    //шаблоны синхрострок 
    uint8_t* line_tmpl8=line_tmpl;
    int  visible_area_pix=SCREEN_W;
    int  begin_HS_pix=328;
    int  size_HS_pix=48;

    uint8_t no__sync8=0b11000000;
    uint8_t hs8= 0b10000000;
    uint8_t vs8= 0b01000000;
    uint8_t vhs8=0b00000000;



    memset(line_tmpl8,no__sync8,LINE_SIZE);
    memset(line_tmpl8+begin_HS_pix,hs8,size_HS_pix);

    line_tmpl8+=LINE_SIZE;    
    memset(line_tmpl8,vs8,LINE_SIZE);
    memset(line_tmpl8+begin_HS_pix,vhs8,size_HS_pix);

    //шаблоны для контрольного регистра
    uint32_t dma_chan_ctrl_tmpl[6];
    dma_chan_ctrl_tmpl[0]=dma_hw->ch[dma_chan].al1_ctrl;
    //32 разрядные данные вместо 16
    dma_chan_ctrl_tmpl[1]=(dma_chan_ctrl_tmpl[0] & ~DMA_CH0_CTRL_TRIG_DATA_SIZE_BITS)| (((uint)DMA_SIZE_32) << DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB);
    
    //32 разрядные данные вместо 16 и отвязка от dreq
    dma_chan_ctrl_tmpl[2]=(dma_chan_ctrl_tmpl[1]  & ~DMA_CH0_CTRL_TRIG_TREQ_SEL_BITS) | (DREQ_FORCE << DMA_CH0_CTRL_TRIG_TREQ_SEL_LSB);;
    
     //32 разрядные данные вместо 16 и без инкремента чтения
    dma_chan_ctrl_tmpl[3]=(dma_chan_ctrl_tmpl[1] & ~DMA_CH0_CTRL_TRIG_INCR_READ_BITS);


    //шаблон для изображения из видеобуфера
    CMD_DMA_TMPL[0][0]=&PIO_VGA_conv->rxf[sm_c];
    CMD_DMA_TMPL[0][1]=&dma_hw->ch[dma_screen_conv_out].al3_read_addr_trig;
    CMD_DMA_TMPL[0][2]=SCREEN_W/2;
    CMD_DMA_TMPL[0][3]=dma_chan_ctrl_tmpl[3];//регистр управления настроить(передача 32 разр данных)
    //шаблон синхросигнала на строке изображения
    line_tmpl8=line_tmpl;
    line_tmpl8+=visible_area_pix;
    CMD_DMA_TMPL[1][0]=line_tmpl8;
    CMD_DMA_TMPL[1][1]=&PIO_VGA->txf[sm];
    CMD_DMA_TMPL[1][2]=(LINE_SIZE-visible_area_pix)/2;
    CMD_DMA_TMPL[1][3]=dma_chan_ctrl_tmpl[0];//регистр управления настроить

    //шаблон  строки содержащей строчную синхру
    line_tmpl8=line_tmpl;
    CMD_DMA_TMPL[2][0]=line_tmpl8;
    CMD_DMA_TMPL[2][1]=&PIO_VGA->txf[sm];
    CMD_DMA_TMPL[2][2]=LINE_SIZE/2;
    CMD_DMA_TMPL[2][3]=dma_chan_ctrl_tmpl[0];//регистр управления настроить
    //шаблон  строки содержащей строчную и кадровую синхру
    line_tmpl8+=LINE_SIZE;
    CMD_DMA_TMPL[3][0]=line_tmpl8;
    CMD_DMA_TMPL[3][1]=&PIO_VGA->txf[sm];
    CMD_DMA_TMPL[3][2]=LINE_SIZE/2;
    CMD_DMA_TMPL[3][3]=dma_chan_ctrl_tmpl[0];//регистр управления настроить
    

    //шаблон  перезагружающий dma_screen_conv_cmd
    RELOAD_DATA[0]=screen_CMD_dma;
    CMD_DMA_TMPL[4][0]=&RELOAD_DATA[0];
    CMD_DMA_TMPL[4][1]=&dma_hw->ch[dma_screen_conv_cmd].al3_read_addr_trig;
    CMD_DMA_TMPL[4][2]=1;
    CMD_DMA_TMPL[4][3]= dma_chan_ctrl_tmpl[2];//32 разр данные настроить

    //шаблон  перезагружающий dma_chan_ctrl
    RELOAD_DATA[1]=CMD_DMA0;
    CMD_DMA_TMPL[5][0]=&RELOAD_DATA[1];
    CMD_DMA_TMPL[5][1]=&dma_hw->ch[dma_chan_ctrl].read_addr;
    CMD_DMA_TMPL[5][2]=1;
    CMD_DMA_TMPL[5][3]=dma_chan_ctrl_tmpl[1];//32 разр данные настроить

    //формирование основного управляющего массива по линиям
       const  int  begin_VS_line=490;
       const  int  size_VS_line=2;
        int i_line=0;
        int inx=0;
    for(;i_line<FRAME_LINES;i_line++)
    {
       if(i_line<VISIBLE_LINES)
            {
                  CMD_DMA0[inx++]=CMD_DMA_TMPL[0];
                  CMD_DMA0[inx++]=CMD_DMA_TMPL[1];
                //CMD_DMA0[inx++]=CMD_DMA_TMPL[2];

                continue;
            }
      if (i_line==VISIBLE_LINES)    CMD_DMA0[inx++]=CMD_DMA_TMPL[4];

      if((i_line>=begin_VS_line) && (i_line<(begin_VS_line+size_VS_line)))
            {
                CMD_DMA0[inx++]=CMD_DMA_TMPL[3];
                continue;
            }
      CMD_DMA0[inx++]=CMD_DMA_TMPL[2];
    
        
    }    
    

    
    // CMD_DMA0[inx++]=CMD_DMA_TMPL[4];
  
    CMD_DMA0[inx++]=CMD_DMA_TMPL[5];

     dma_start_channel_mask((1u << dma_chan_ctrl)) ;




    // set_sys_clock_khz(256000, true);
    // sleep_ms(10); 
    // fdiv=8;


  
    // //определение адреса во флеше с данными для вывода по DMA
    // if (((uint32_t)flash_data_lines%FLASH_SECTOR_SIZE)==0)
    //     data_lines_addr=(uint32_t)flash_data_lines;
    // else 
    //     data_lines_addr=((uint32_t)flash_data_lines&(~(FLASH_SECTOR_SIZE-1)))+FLASH_SECTOR_SIZE;
    
    // dma_chan_ctrl = dma_claim_unused_channel(true);
    // int dma_chan = dma_claim_unused_channel(true);

    // //шаблоны синхросигналов
    // uint8_t no__sync8=0b11000000;
    // uint8_t hs8=0b10000000;
    // uint8_t vs8=0b01000000;
    // uint8_t vhs8=0;
    // float pix_freq=(25175000.0/2);


    // //854x480
    // // if (is_wide_mode)
    // // {
    // //      pix_freq=(31728500.0/2);
         
    // //     no__sync8=0b01000000;
    // //     hs8=      0b00000000;
    // //     vs8=      0b11000000;
    // //     vhs8=     0b10000000;

    // //     line_size_pix=532;
    // //     visible_area_pix=426;
    // //     begin_HS_pix=436;
    // //     size_HS_pix=44;

    // //     frame_size_line=497;
    // //     visible_area_line=480;
    // //     begin_VS_line=490;
    // //     size_VS_line=3;


    // // }

    // //720x480
    //  if (is_wide_mode)
    // {
    //     #define VS_P (0)
    //     #define HS_P (0)
    //      pix_freq=(27700000.0/2);
         
    //     no__sync8=((VS_P^1)<<7)|((HS_P^1)<<6);
    //     hs8=      ((VS_P^1)<<7)|((HS_P^0)<<6);;
    //     vs8=      ((VS_P^0)<<7)|((HS_P^1)<<6);;
    //     vhs8=     ((VS_P^0)<<7)|((HS_P^0)<<6);;

    //     line_size_pix=440;
    //     visible_area_pix=360;
    //     begin_HS_pix=384;
    //     size_HS_pix=20;

    //     frame_size_line=525;
    //     visible_area_line=480;
    //     begin_VS_line=489;
    //     size_VS_line=3;


    // }



    // uint8_t* buf8=(uint8_t*)sync_tmpl;
    // memset(buf8,no__sync8,LINE_SIZE_STEP);
    // memset(buf8+begin_HS_pix,hs8,size_HS_pix);

    // buf8+=LINE_SIZE_STEP;    
    // memset(buf8,vs8,LINE_SIZE_STEP);
    // memset(buf8+begin_HS_pix,vhs8,size_HS_pix);

    // //шаблоны видимых строк
    // buf8=line_buf;
    // for(int i=0;i<BUF_LINES_SIZE;i++)
    // {
    //     visible_line[i]=buf8;
    //     memset(buf8,no__sync8,LINE_SIZE_STEP);
    //     memset(buf8+begin_HS_pix,hs8,size_HS_pix);
    //     buf8+=LINE_SIZE_STEP;    
    // }

    // //test
    // buf8=line_buf;
    // memset(buf8,no__sync8|0b110000,visible_area_pix);
    // buf8+=LINE_SIZE_STEP;
    // memset(buf8,no__sync8|0b001100,visible_area_pix);
    // buf8+=LINE_SIZE_STEP;
    // memset(buf8,no__sync8|0b000011,visible_area_pix);

    // //для каждого видеорежима свои синхросигналы
    // for(int i=0;i<256;i++) 
    //     {
    //         uint8_t ci=i;

          
            
    //         uint8_t ch=(ci&0xf0)>>4;
    //         uint8_t cl=ci&0xf;

    //         //убрать ярко-чёрный
    //         if (ch==0x08) ch=0;
    //         if (cl==0x08) cl=0;
            
    //         //конвертация в 6 битный формат цвета
    //         ch=zx_color_6b[ch&0xf];
    //         cl=zx_color_6b[cl&0xf];

    //           colors[i]=((ch|no__sync8)<<8)|(cl|no__sync8);
    //         //  colors[i]=((ch)<<8)|(cl)|0xc0c0;

    //     };


    // //пиксельклок для SM 
    // fdiv=clock_get_hz(clk_sys)/pix_freq;//частота VGA по умолчанию
    // uint32_t div32=(uint32_t) (fdiv * (1 << 16)+0.5);
    // PIO_VGA->sm[sm].clkdiv=div32&0xfffff000; //делитель для конкретной sm 
    // //делитель можно загрубить, чтобы уменьшить дрожание pll напр. наложив 0xfffff000
    // //инициализация DMA



    // //основной ДМА канал для данных
    // dma_channel_config c0 = dma_channel_get_default_config(dma_chan);
    // channel_config_set_transfer_data_size(&c0, DMA_SIZE_32);

    // channel_config_set_read_increment(&c0, true);
    // channel_config_set_write_increment(&c0, false);

    // uint dreq=DREQ_PIO1_TX0+sm;
    // if (PIO_VGA==pio0) dreq=DREQ_PIO0_TX0+sm;

    // channel_config_set_dreq(&c0, dreq);
    // channel_config_set_chain_to(&c0, dma_chan_ctrl);                        // chain to other channel
   
    // dma_channel_configure(
    //     dma_chan,
    //     &c0,
    //     &PIO_VGA->txf[sm], // Write address 
    //     line_buf,             // read address 
    //     line_size_pix/4, //
    //     false             // Don't start yet
    // );
    // //канал DMA для контроля основного канала
    // dma_channel_config c1 = dma_channel_get_default_config(dma_chan_ctrl);
    // channel_config_set_transfer_data_size(&c1, DMA_SIZE_32);
    
    // channel_config_set_read_increment(&c1, true);
    // channel_config_set_write_increment(&c1, true);
    
    // // channel_config_set_chain_to(&c1, dma_chan);                         // chain to other channel
    // //channel_config_set_dreq(&c1, DREQ_PIO0_TX0);

    // channel_config_set_ring(&c1,true,4);

    // dma_channel_configure(
    //     dma_chan_ctrl,
    //     &c1,
    //     &dma_hw->ch[dma_chan].read_addr, // Write address 
    //     data_lines_addr,             // read address
    //     4, // 
    //     false             // Don't start yet
    // );
    // //dma_channel_set_read_addr(dma_chan, &DMA_BUF_ADDR[0], false);

    // dma_channel_set_irq0_enabled(dma_chan_ctrl, true);
    


    // reload_DMA[0]=data_lines_addr;
    // const uint DMA_DATA_LEN=800*4;
    //  uint32_t data_lines[DMA_DATA_LEN];
    // //данные для программирования DMA передачи
    // uint8_t* sync_buf8=sync_tmpl;
    // int i_line=0;
    // for(;i_line<frame_size_line;i_line++)
    // {
    //    if(i_line<visible_area_line)
    //         {
    //             data_lines[i_line*4]=visible_line[(i_line>>1)%BUF_LINES_SIZE];
    //             data_lines[i_line*4+1]=&PIO_VGA->txf[sm];
    //             data_lines[i_line*4+2]=line_size_pix/4;
    //             data_lines[i_line*4+3]=dma_hw->ch[dma_chan].al2_ctrl;
    //             continue;
    //         }
    //   if((i_line>=begin_VS_line) && (i_line<(begin_VS_line+size_VS_line)))
    //         {
    //             data_lines[i_line*4]=sync_buf8+LINE_SIZE_STEP;
    //             data_lines[i_line*4+1]=&PIO_VGA->txf[sm];
    //             data_lines[i_line*4+2]=line_size_pix/4;
    //             data_lines[i_line*4+3]=dma_hw->ch[dma_chan].al2_ctrl;
    //             continue;
    //         }
    //   data_lines[i_line*4]=sync_buf8;
    //   data_lines[i_line*4+1]=&PIO_VGA->txf[sm];
    //   data_lines[i_line*4+2]=line_size_pix/4;
    //   data_lines[i_line*4+3]=dma_hw->ch[dma_chan].al2_ctrl;
        
    // }

    //  data_lines[i_line*4]=reload_DMA;
    //  data_lines[i_line*4+1]=&dma_hw->ch[dma_chan_ctrl].read_addr;
    //  data_lines[i_line*4+2]=1;
    //  data_lines[i_line*4+3]=dma_hw->ch[dma_chan].al2_ctrl;
    

    // write_flash(data_lines_addr,data_lines,DMA_DATA_LEN*4);

    // //test
    // // sleep_ms(2000);
    // // printf("0x%08x\n",data_lines[0]);
    // // printf("0x%08x\n",((uint32_t*)data_lines_addr)[0]);
    

    
    
    // dma_start_channel_mask((1u << dma_chan)) ;

    //  // Configure the processor to run dma_handler() when DMA IRQ 0 is asserted

    // sleep_ms(10);
    // irq_set_exclusive_handler(DMA_IRQ_0, dma_handler_VGA);

    // irq_set_enabled(DMA_IRQ_0, true);




    // G_PRINTF_INFO("init VGA video out\n");
}