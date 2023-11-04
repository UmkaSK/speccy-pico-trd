#include "string.h"
#include "stdbool.h"

#include "zx_machine.h"
#include "aySoundSoft.h"

#include "util.h"
#include "rom/pentagon_trdos504tm.h"

#include "z80.h"
#include "../g_config.h"
#include "hardware/structs/systick.h"
#include "util_tap.h"

//#define Z80_DEBUG

//bool z80_gen_nmi_from_main = false;
bool im_z80_stop = false;
bool im_ready_loading = false;
bool vbuf_en=true;

////////////////////////////////
// tr-dos
#include "trdos.h"
bool trdos=0;
////////////////////////////////

//uint8_t zx_machine_last_out_7ffd;
uint8_t zx_machine_last_out_fffd;

////цвета спектрума в формате 6 бит
// uint8_t zx_color[]={
//         0b00000000,
//         0b00000010,
//         0b00100000,
//         0b00100010,
//         0b00001000,
//         0b00001010,
//         0b00101000,
//         0b00101010,
//         0b00000000,
//         0b00000011,
//         0b00110000,
//         0b00110011,
//         0b00001100,
//         0b00001111,
//         0b00111100,
//         0b00111111
//     };
//переменные состояния спектрума
volatile z80 cpu;

ZX_Input_t zx_input;
bool zx_state_48k_MODE_BLOCK=false;
uint8_t zx_RAM_bank_active=3;
#if (ZX_BPP==4)
	uint8_t zx_Border_color=0x00;
	// uint32_t zx_Border_color32=0x00;
	static uint32_t zx_colors_2_pix32[384];//предпосчитанные сочетания 2 цветов
	static uint8_t* zx_colors_2_pix=(uint8_t*)&zx_colors_2_pix32;//предпосчитанные сочетания 2 цветов
#endif
#if (ZX_BPP==8)
	uint16_t zx_Border_color=0x00;
	static uint16_t zx_colors_2_pix[384*4];//предпосчитанные сочетания 2 цветов
#endif
uint8_t* zx_cpu_ram[4];//Адреса 4х областей памяти CPU при использовании страниц
uint8_t* zx_video_ram;//4 области памяти CPU

uint8_t* zx_ram_bank[8];//Хранит адреса 8ми банков памяти
uint8_t* zx_rom_bank[4];//Адреса 4х областей ПЗУ (48к 128к TRDOS и резерв для какого либо режима(типа тест))

typedef struct zx_vbuf_t
{
	uint8_t* data;
	bool is_displayed;
}zx_vbuf_t;

zx_vbuf_t zx_vbuf[ZX_NUM_GBUF];
zx_vbuf_t* zx_vbuf_active;


//выделение памяти может быть изменено в зависимости от платформы
uint8_t RAM[16384*8]; //Реальная память куском 128Кб
//uint8_t VBUFS[ZX_SCREENW*ZX_SCREENH*ZX_NUM_GBUF*ZX_BPP/8];

//

uint8_t FAST_FUNC(zx_keyboardDecode)(uint8_t addrH)
{
	
	//быстрый опрос
	
	switch (addrH)
	{
		case 0b11111111: return 0xff;break;
		case 0b11111110: return ~zx_input.kb_data[0];break;
		case 0b11111101: return ~zx_input.kb_data[1];break;
		case 0b11111011: return ~zx_input.kb_data[2];break;
		case 0b11110111: return ~zx_input.kb_data[3];break;
		case 0b11101111: return ~zx_input.kb_data[4];break;
		case 0b11011111: return ~zx_input.kb_data[5];break;
		case 0b10111111: return ~zx_input.kb_data[6];break;
		case 0b01111111: return ~zx_input.kb_data[7];break;
		
	}
	
	//несколько адресных линий в 0 - медленный опрос
	uint8_t dataOut=0;
	
	for(uint8_t i=0;i<8;i++)
	{
		if ((addrH&1)==0) dataOut|=zx_input.kb_data[i];//работаем в режиме нажатая клавиша=1
		addrH>>=1;
	};
	
	return ~dataOut;//инверсия, т.к. для спектрума нажатая клавиша = 0;
};


//функции чтения памяти и ввода-вывода
static uint8_t FAST_FUNC(read_z80)(void* userdata, uint16_t addr)
{
	if (addr<16384) return zx_cpu_ram[0][addr];
	if (addr<32768) return zx_cpu_ram[1][addr-16384];
	if (addr<49152) return zx_cpu_ram[2][addr-32768];
	return zx_cpu_ram[3][addr-49152];
}

static void FAST_FUNC(write_z80)(void* userdata, uint16_t addr, uint8_t val)
{
	if (addr<16384) return;//запрещаем писать в ПЗУ
	if (addr<32768) {zx_cpu_ram[1][addr-16384]=val;return;};
	if (addr<49152) {zx_cpu_ram[2][addr-32768]=val;return;};
	zx_cpu_ram[3][addr-49152]=val;
}

unsigned long prev_ticks, cur_ticks;

//IN!
static uint8_t FAST_FUNC(in_z80)(z80* const z, uint8_t port) {
	
	uint8_t portH=z->_hi_addr_port;
	uint8_t portL=port;
	uint16_t port16=(portH<<8)|portL;

/////////////////////////////////////////////////////////
	if (trdos) // если это tr-dos
	{
        uint8_t VG = 0xff;
		if ((portL & 0x1f) > 0)
		{

			uint8_t i = (portL >> 4) & 0x0E;
			if (i < 8)
			{
				ChipVG93(i, &VG);
			}
			else
			{
				if (i == 0x0E)
					ChipVG93(8, &VG);
			}
			return VG;
		}
	}
///////////////////////////////////////////////////////////////////






	if (port16&1)
	{
		uint16_t not_port16=~port16;

		
		if ((port16&0x20)==0x0) {return zx_input.kempston;}//kempston{return 0xff;};//
		if ((port16&0xc002)==0xc000)return AY_get_reg();  //fffd
		
		
	}
	else
	{
		//загрузка с магнитофона и опрос клавиатуры
		
		//if (port16!=0x7FFE) printf(": %X ", port16);
		/*
			if ((port)==0xFE)
			{
			cur_ticks=cpu.cyc;
			printf("%d\n",cur_ticks-prev_ticks);
			prev_ticks=cur_ticks;
			}
		*/
		if (hw_zx_get_bit_LOAD())
		
		{
			
			uint8_t out_data=zx_keyboardDecode(portH);
			
			return out_data;
		}
		else
		{
			
			uint8_t out_data=zx_keyboardDecode(portH);
			return(out_data&0b10111111);
		};
		
		
	}
	
	
	
	return 0xFF;
}

uint8_t zx_7ffd_lastOut=0;
void zx_machine_set_7ffd_out(uint8_t val)
{
	zx_RAM_bank_active=(val&0x7);
	//            printf("7FFD Page&data :%02X\n",val);
	
	//            zx_cpu_ram[3]=zx_ram_bank[val&0x7];
	zx_cpu_ram[3]=zx_ram_bank[zx_RAM_bank_active];
	
	if (val&8) zx_video_ram=zx_ram_bank[7];else zx_video_ram=zx_ram_bank[5];
    zx_cpu_ram[0]=zx_rom_bank[(val & 0x10)>>4];
	if (val&32) zx_state_48k_MODE_BLOCK=true; // 6bit = 1 48k mode block
	
};
uint8_t zx_machine_get_7ffd_lastOut(){return zx_7ffd_lastOut;}

//OUT!
static void FAST_FUNC(out_z80)(z80* const z, uint8_t port, uint8_t val) {
	uint8_t portH=z->_hi_addr_port;
	uint8_t portL=port;
	uint16_t port16=(portH<<8)|portL;
//////////////////////////////////////////////////////////////

	if (trdos) // если это tr-dos
	{

    if ((portL & 0x1f)>0){
	uint8_t VG = val;
    uint8_t btemp=(portL >> 4);
    if (btemp<8) ChipVG93(btemp,&VG);
	else{
    	if (btemp==0x0F) ChipVG93(9,&VG);
		}
	//return;	/////////////////////////
	}
	}

/////////////////////////////////////////////////////////////


	if (port16&1)
	{
		
		uint16_t not_port16=~port16;
		//чип AY
		//          if (port16 == 0xFFFD){AY_select_reg(val);return;} //fffd
		//           if (port16 == 0xBFFD){AY_set_reg(val);return;} //bffd
		
		if (((not_port16&0x0002)==0x0002)&&((port16&0xc000)==0xc000)) 
		{ 
			//if (port16!=0xFFFD)G_PRINTF("port FFFD: %x=%x\n",port16,val);     
			//G_PRINTF("F:%x ",val);
			AY_select_reg(val); 
			zx_machine_last_out_fffd = val;
			return;
		} //fffd
		
		if (((not_port16&0x4002)==0x4002)&&((port16&0x8000)==0x8000)) 
		{ 
			//if (port16!=0xBFFD)G_PRINTF("port BFFD: %x=%x\n",port16,val);
			//G_PRINTF("B:%x ",val);
			AY_set_reg(val);
			return;
		} //bffd
		
		
		//Расширение памяти и экран Spectrum-128
		
		//  if ((port16==0x7FFD) && ((not_port16 & 0x8002) == 0x8002))
		//  z80_gen_nmi_from_main=true;
		//if ((port16==0x7FFD))
		if (((not_port16 & 0x8002) == 0x8002))//7ffd
		
		{
			// if (port16!=0x7FFD) 
			//    G_PRINTF("port16 must be 7FFD, but it is %X=val(%x)\n",port16,val);
			
			//G_PRINTF("[%X=%x] \n",port16,val);
			
			//zx_machine_last_out_7ffd = val;
			zx_7ffd_lastOut=val;
			
			if (zx_state_48k_MODE_BLOCK) return; //защёлка для 48к, запрешает манипуляцию банками
			
			
			//переключение банка памяти
			zx_RAM_bank_active=(val&0x7);
			//            printf("7FFD Page&data :%02X\n",val);
			
			//            zx_cpu_ram[3]=zx_ram_bank[val&0x7];
			zx_cpu_ram[3]=zx_ram_bank[zx_RAM_bank_active];
			
			if (val&8) zx_video_ram=zx_ram_bank[7];else zx_video_ram=zx_ram_bank[5];
			zx_cpu_ram[0]=zx_rom_bank[(val & 0x10)>>4];
			if (val&32) zx_state_48k_MODE_BLOCK=true; // 6bit = 1 48k mode block
			return;
			//
		}; 
		
		
		
	}
	else
	{
		#define OUT_SND_MASK   (0b00011000)
		hw_zx_set_snd_out (val&0b10000); // 10000
		hw_zx_set_save_out(val&0b01000); // 01000
		//    if ((val&OUT_SND_MASK)!=oldSoundOutValue) {outSoundOutState^=1;digitalWrite(ZX_OUT_BEEP,outSoundOutState);};
		//    oldSoundOutValue=value&OUT_SND_MASK;
		//
		//digital_Write(ZX_OUT_BEEP,(value&0b00010000)?1:0); //если только звук без выхода записи#if (ZX_NUM_GBUF==1)
		#if (ZX_BPP==4)
			zx_Border_color=((val&0x7)<<4)|(val&0x7);//дублируем для 4 битного видеобуфера
			// zx_Border_color32=(zx_Border_color<<24)|(zx_Border_color<<16)|(zx_Border_color<<8)|(zx_Border_color<<0);
			
			#else
			zx_Border_color=(zx_color[val&7]<<8)|zx_color[val&7];
		#endif
		
		
	}
	
	
}


//
//uint8_t ROM_BUF[2][16384];
void zx_machine_init()
{
	//привязка реальной RAM памяти к банкам
	for(int i=0;i<8;i++)
	{
		zx_ram_bank[i]=&RAM[i*16384];
	}
	//    привязка ROM памяти

	zx_rom_bank[3]=&ROM_P[0*16384];//нулевая банка пзу
    zx_rom_bank[2]=&ROM_P[1*16384];//TRDOS
    zx_rom_bank[0]=&ROM_P[2*16384];//128k 
	zx_rom_bank[1]=&ROM_P[3*16384];//48k 
	
	
	zx_cpu_ram[0]=zx_rom_bank[0]; // 0x0000 - 0x3FFF
	zx_cpu_ram[1]=zx_ram_bank[5]; // 0x4000 - 0x7FFF
	zx_cpu_ram[2]=zx_ram_bank[2]; // 0x8000 - 0xBFFF
	zx_cpu_ram[3]=zx_ram_bank[0]; // 0xC000 - 0x7FFF
	zx_video_ram=zx_ram_bank[5];
	zx_RAM_bank_active=0;
	zx_state_48k_MODE_BLOCK=false;
	//printf("\nInit cpu_ram %04X rom_bank %04X\n", zx_cpu_ram[0], zx_rom_bank[0]);
	//выделение графических буферов
	// for(int i=0;i<ZX_NUM_GBUF;i++)
	// {
	//     zx_vbuf[i].is_displayed=true;
	//     zx_vbuf[i].data=&VBUFS[i*(ZX_SCREENW*ZX_SCREENH*ZX_BPP/8)];
	// }
	zx_vbuf[0].is_displayed=true;
	zx_vbuf[0].data=g_gbuf;
	zx_vbuf_active=&zx_vbuf[0];
	
	//инициализация процессора
	
	z80_init(&cpu);
	cpu.read_byte = read_z80;   // Присваиваем процедуру read_z80 структуре z80 (Процедура cpu.readbyte)
	cpu.write_byte = write_z80; // Аналогично
	cpu.port_in = in_z80;       // Аналогично
	cpu.port_out = out_z80;     // Аналогично
	
	
	G_PRINTF_INFO("zx machine initialized\n");
};


void FAST_FUNC(zx_machine_input_set)(ZX_Input_t* input_data){memcpy(&zx_input,input_data,sizeof(ZX_Input_t));};

void zx_machine_reset(){
	
	z80* z=&cpu;
	zx_cpu_ram[0]=zx_rom_bank[0]; // 0x0000 - 0x3FFF
	zx_cpu_ram[1]=zx_ram_bank[5]; // 0x4000 - 0x7FFF
	zx_cpu_ram[2]=zx_ram_bank[2]; // 0x8000 - 0xBFFF
	zx_cpu_ram[3]=zx_ram_bank[0]; // 0xC000 - 0x7FFF
	zx_video_ram=zx_ram_bank[5];
	zx_RAM_bank_active=0;
	zx_state_48k_MODE_BLOCK=false;
	
	zx_vbuf[0].is_displayed=true;
	zx_vbuf[0].data=g_gbuf;
	zx_vbuf_active=&zx_vbuf[0];                        
	
	z->pc = 0;
	z->sp = 0xFFFF;
	z->ix = 0;
	z->iy = 0;
	z->mem_ptr = 0;
	
	// af and sp are set to 0xFFFF after reset,
	// and the other values are undefined (z80-documented)
	z->a = 0xFF;
	z->b = 0;
	z->c = 0;
	z->d = 0;
	z->e = 0;
	z->h = 0;
	z->l = 0;
	
	z->a_ = 0;
	z->b_ = 0;
	z->c_ = 0;
	z->d_ = 0;
	z->e_ = 0;
	z->h_ = 0;
	z->l_ = 0;
	z->f_ = 0;
	
	z->i = 0;
	z->r = 0;
	
	z->sf = 1;
	z->zf = 1;
	z->yf = 1;
	z->hf = 1;
	z->xf = 1;
	z->pf = 1;
	z->nf = 1;
	z->cf = 1;
	
	z->iff_delay = 0;
	z->interrupt_mode = 0;
	z->iff1 = 0;
	z->iff2 = 0;
	z->halted = 0;
	z->int_pending = 0;
	z->nmi_pending = 0;
	z->int_data = 0;
	AY_reset();
};

uint8_t* FAST_FUNC(zx_machine_screen_get)(uint8_t* current_screen)
{
	#if (ZX_NUM_GBUF==1)
		return zx_vbuf[0].data; //если буфер 1, то вариантов нет
		#else
		//для нескольких буферов надо возвращать ранее неотображённый, если найдётся
		uint8_t* out_data=current_screen;
		zx_vbuf_t* current_out_zx_vbuf=NULL;
		for(int i=0;i<ZX_NUM_GBUF;i++)
		{
			if (zx_vbuf[i].data==current_screen) current_out_zx_vbuf=&zx_vbuf[i];//запомнить текущий буфер
			if (!zx_vbuf[i].is_displayed) out_data=zx_vbuf[i].data;//неотображённый ещё буфер
			
		}
		//если нашли неотображённый ранее буфер экрана, прошлый надо освободить для отрисовки
		if ((out_data!=current_screen)&&(current_out_zx_vbuf!=NULL)) current_out_zx_vbuf->is_displayed=true;
		
		
		return out_data;
		
	#endif
};

void FAST_FUNC(zx_machine_flashATTR)(void)
{
	static bool stateFlash=true;
	stateFlash^=1;
	#if ZX_BPP==4
		if (stateFlash) memcpy(zx_colors_2_pix+512,zx_colors_2_pix,512); else memcpy(zx_colors_2_pix+512,zx_colors_2_pix+1024,512);
		#else
		if (stateFlash) memcpy(zx_colors_2_pix+512*2,zx_colors_2_pix,512*2); else memcpy(zx_colors_2_pix+512*2,zx_colors_2_pix+1024*2,512*2);
	#endif
	
}
//инициализация массива предпосчитанных цветов
void init_zx_2_pix_buffer()
{
	for(uint16_t i=0;i<384;i++)
	{
		uint8_t color=(uint8_t)i&0x7f;
		uint8_t color0=(color>>3)&0xf;
		uint8_t color1=(color&7)|(color0&0x08);
		
		//убрать ярко чёрный
		//  if (color0==0x80) color0=0;
		//  if (color1==0x80) color1=0;
		
		if (i>128)
		{
			//инверсные цвета для мигания
			uint8_t color_tmp=color0;
			color0=color1;
			color1=color_tmp;
			
		}
		
		for(uint8_t k=0;k<4;k++)
		{
			switch (k)
			{
				case 0:
				
				zx_colors_2_pix[i*4+k]=(ZX_BPP==4)?(color0<<4)|color0:(zx_color[color0]<<8)|zx_color[color0];
				
				break;
				case 2:
				zx_colors_2_pix[i*4+k]=(ZX_BPP==4)?(color0<<4)|color1:(zx_color[color0]<<8)|zx_color[color1];
				
				break;
				case 1:
				zx_colors_2_pix[i*4+k]=(ZX_BPP==4)?(color1<<4)|color0:(zx_color[color1]<<8)|zx_color[color0];
				
				break;
				case 3:
				zx_colors_2_pix[i*4+k]=(ZX_BPP==4)?(color1<<4)|color1:(zx_color[color1]<<8)|zx_color[color1];
				
				
				break;
			}
		}
		
	}
	
}

uint8_t* active_screen_buf=NULL;



void FAST_FUNC(zx_machine_main_loop_start)(){
	//void (zx_machine_main_loop_start)(){
	//void __not_in_flash_func(zx_machine_main_loop_start)(){
	
	//char icon[2];
	//переменные для отрисовки экрана
	const int sh_y=56;
	const int sh_x=104;
	uint32_t inx_tick_screen=0;
	uint64_t tick_cpu=0; // Количество тактов до выполнения команды Z80
	uint32_t x=0;
	uint32_t y=0;
	
	init_zx_2_pix_buffer();
	#if (ZX_BPP==4)
		uint8_t* p_scr_str_buf=NULL;
		#else
		uint16_t* p_scr_str_buf=NULL;
	#endif
	uint8_t* p_zx_video_ram=NULL;
	uint8_t* p_zx_video_ramATTR=NULL;
	
	G_PRINTF_INFO("zx mashine starting\n");
	
	// uint64_t dst_time_ns=ext_get_ns();
	uint32_t d_dst_time_ticks=0; // Количесто тактов реального процессора на текущую выполненную команду Z80
	uint32_t t0_time_ticks=0;    // Количество реальных тактов процессора после запуска машины Z80
	
	int ticks_per_cycle=72;//72;     //от 254МГц: 72 - 3.5МГц, 63 - 4Мгц;
	int time_cycle_z80_ns=285;  // Реальное время одного цикла CPU
	//int time_cycle_z80_ns=285;  // Реальное время одного цикла CPU
	//G_PRINTF_DEBUG("time_ns=%lu trg time_ns=%lu\n",ext_get_ns(),dst_time_ns);//test
	systick_hw->csr = 0x5;
	systick_hw->rvr = 0xFFFFFF;
	
	G_PRINTF_DEBUG("time_tick=%ld dtime tick=%d\n",get_ticks(),(1-0xffff00)&0xffffff);//test
	G_PRINTF_DEBUG("time_tick=%ld dtime tick=%d\n",get_ticks(),(1-0xffff00)&0xffffff);//test
	
	//        bool get_next_PC = 0;
	//uint16_t get_next_codes = 0;
	#ifdef Z80_DEBUG
		bool ttest = false; // Флаг для вызова дебажной информации
	#endif
	
	//init_screen(g_gbuf,320,240);
	active_screen_buf=g_gbuf;
	p_scr_str_buf=active_screen_buf; 
	
	//смещение начала изображения от прерывания
	const int shift_img=(16+40)*224+44;////8888;////Пентагон=(16+40)*224+48;
	//вспомогательный индекс такта внутри картинки
	int draw_img_inx=0;
	const int ticks_per_frame=71680 ;// 71680- Пентагон //70908 - 128 +2A
	const int T_per_line = 224;
	bool int_en=true;
	//работа с аттрибутами цвета
	register uint8_t old_c_attr=0;
	register uint8_t old_zx_pix8=0;
	register uint32_t colorBuf;
	
	while(1){
		
		while (im_z80_stop){
			sleep_ms(1);
			if (!im_ready_loading) im_ready_loading = true;
			// inx_tick_screen=0;
			
			
			#ifdef Z80_DEBUG
				ttest=true;
				
			#endif
			cpu.int_pending = false;
		}
		#ifdef Z80_DEBUG
			if (ttest){
				sleep_ms(10);
				z80_debug_output(&cpu);
				printf("R_Bank: %d\n", zx_RAM_bank_active);
				ttest=false;
			}
		#endif
		
		//if (inx_tick_screen<36) z80_gen_int(&cpu,0xFF);
		// while (ext_get_ns()<dst_time_ns) ;//ext_delay_us(1);
		// Цикл ождания пока количество потраченных тактов рального процессора
		// меньше количества расчетных тактов реального процессора на команду Z80
		while (((get_ticks()-t0_time_ticks)&0xffffff)<d_dst_time_ticks);
		
		//
		if ((inx_tick_screen<32)&&(int_en)) {z80_gen_int(&cpu,0xFF);int_en=false;}
		
		
		t0_time_ticks=(t0_time_ticks+d_dst_time_ticks)&0xffffff;         
		tick_cpu=cpu.cyc;                 // Запоминаем количество тактов Z80 до выполнения команды Z80
		z80_step(&cpu);                   // Выполняем очередню команду Z80
///////////////////////////////////////////////////////////////////////////////////////////////////////
// tr-dos
		if (!trdos)
		{
			if (((cpu.pc & 0x3D00) == 0x3D00) && (zx_7ffd_lastOut & 0x10)) // trdos работает с BASIC48 D4 = 1

			{
				trdos = true;
				zx_cpu_ram[0] = zx_rom_bank[2]; // ROM  TRDOS
			}
		}

		if ((cpu.pc > 0x3FFF) && (trdos))
		{
			trdos = false;

			zx_cpu_ram[0] = zx_rom_bank[(zx_7ffd_lastOut & 0x10) >> 4];
		}

///////////////////////////////////////////////////////////////////////////////////////////////////////		
		if(tap_loader_active){
			//printf("Tap loader active:%d\n",cpu.pc);
			if(tape_autoload_status==2){
				switch (cpu.pc) {
					case 0x0556: // START LOAD
					RomLoading=true;
					if (TapeStatus!=TAPE_LOADING && tapeFileSize>0) TAP_Play();
					//printf("ROM TAPE_LOADING\n");
					// printf("TapeStatus: %u\n", Tape::tapeStatus);
					break;
					case 0x04d0: // START SAVE (used for rerouting mic out to speaker in Ports.cpp)
						SaveStatus=TAPE_SAVING; 
						// printf("------\n");
						// printf("SaveStatus: %u\n", Tape::SaveStatus);
					break;
					case 0x053f: // END LOAD / SAVE
					RomLoading=false;
					//printf("Tap load autostop %d\n",TapeStatus);
					if (TapeStatus!=TAPE_STOPPED){
						if (cpu.cf) {
							TapeStatus=TAPE_PAUSED;
							//printf("ROM TAPE_PAUSED %X\n",cpu.cf);
						} else{
							TapeStatus=TAPE_STOPPED;
							//printf("ROM TAPE_STOPPED %X\n",cpu.cf);
							tap_loader_active = false;
						}
						SaveStatus=SAVE_STOPPED;
					}
					
					
					/*
						printf("TapeStatus: %u\n", Tape::tapeStatus);
						printf("SaveStatus: %u\n", Tape::SaveStatus);
						printf("Carry Flag: %u\n", Z80::isCarryFlag());            
						printf("------\n");
					*/
					break;
				}
			}
		}
		
		
		uint32_t dt_cpu=cpu.cyc-tick_cpu; // Вычисляем количество тактов Z80 на выполненную команду.
		//dst_time_ns+=time_cycle_z80_ns*dt_cpu;
		d_dst_time_ticks=dt_cpu*ticks_per_cycle; // Расчетное количесто тактов реального процессора на воплненную команду Z80
		inx_tick_screen+=dt_cpu;          //Увеличиваем на количество тактов Z80 на текущую выполненную команду.
		
		
		
		if (inx_tick_screen>=ticks_per_frame){       // Если прошла 1/50 сек, 71680 тактов процессора Z80
			int_en=true;
			inx_tick_screen-=ticks_per_frame; //Такты Z80 1/50 секунды
			x=0;y=0;
			draw_img_inx=0; 
			p_scr_str_buf=active_screen_buf; 
			if (inx_tick_screen==0) continue;
		};
		/*
			// Если нажаликлавишу NMI
			if (z80_gen_nmi_from_main)
			{
			//z80_gen_nmi(&cpu);
			z80_gen_nmi_from_main = false;
			}
		*/
		//if (active_screen_buf==NULL) continue;
		if (!vbuf_en) continue;
		//новая прорисовка
		register int img_inx=(inx_tick_screen-shift_img);
		if (img_inx<0 || (img_inx>=(T_per_line*240))){ //область изображения, если вне, то не рисуем
			continue;
		}
		
		//смещения бордера
		const int dy=24;
		const int dx=32;
		
		for(;draw_img_inx<img_inx;){
			
			if (x==T_per_line*2) {
				x=0;
				y++;
				int ys=y-dy;//номер строки изображения
				p_zx_video_ram=zx_video_ram+(((ys&0b11000000)|((ys>>3)&7)|((ys&7)<<3))<<5);
				//указатель на начало строки байтов цветовых аттрибутов
				p_zx_video_ramATTR=zx_video_ram+(6144+((ys<<2)&0xFFE0));
				
			}; 
			if (x>=(SCREEN_W)||y>=(SCREEN_H))
			{
				x+=8;
				draw_img_inx+=4;
				continue;
			} 
			if((y<dy)||(y>=192+dy)||(x>=256+dx)||(x<dx)){//условия для бордера
				int i_c;
				if (x<dx) i_c=MIN((dx-x)/2,img_inx-draw_img_inx);
				else i_c=MIN((SCREEN_W-x)/2,img_inx-draw_img_inx);
				
				register uint8_t bc=zx_Border_color;     
				for(int i=i_c;i--;) *p_scr_str_buf++=bc;
				//  p_scr_str_buf+=i_c;//test
				draw_img_inx+=i_c;
				x+=i_c<<1;
				
				continue;
				
				
			}
			
			uint8_t c_attr=*p_zx_video_ramATTR++;
			uint8_t zx_pix8=*p_zx_video_ram++;
			
			if (old_c_attr!=c_attr)//если аттрибуты цвета не поменялись - используем последовательность с прошлого шага
			{
				// uint8_t* zx_colors_2_pix_current=zx_colors_2_pix+(4*(c_attr));
				// colorBuf=zx_colors_2_pix_current);
				colorBuf=*((zx_colors_2_pix32+c_attr));
				old_c_attr=c_attr;
				
			}
			
			
			//вывод блока из 8 пикселей
			*p_scr_str_buf++=colorBuf>>(((zx_pix8)&0xc0)>>3);
			*p_scr_str_buf++=colorBuf>>(((zx_pix8)&0x30)>>1);
			*p_scr_str_buf++=colorBuf>>(((zx_pix8)&0x0c)<<1);
			*p_scr_str_buf++=colorBuf>>(((zx_pix8)&0x03)<<3);
			
			
			// p_scr_str_buf+=4;//test
			x+=8;
			draw_img_inx+=4;
			
		};
		continue; 
	}//while(1)
};

// void zx_machine_set_vbuf(uint8_t* vbuf){active_screen_buf=vbuf;};
void zx_machine_enable_vbuf(bool en_vbuf){
	vbuf_en=en_vbuf;
};


/*uint8_t zx_machine_tape_active(){
	if(tap_loader_active){
	printf("GET autostart\n");
	return tape_active;
	}
	return 0;
};*/

/*
	void zx_machine_set_tape_pos(uint16_t pos){
	tape_position = pos;
	};
*/
