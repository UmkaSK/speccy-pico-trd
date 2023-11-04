#include "hardware/dma.h"
#include "ps2.h"
#include "kb_u_codes.h"
#include "g_config.h"
#include "string.h"

#include "hardware/clocks.h"

#include "hardware/structs/pll.h"
#include "hardware/structs/systick.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "PIO_program1.h"


#if !PICO_NO_HARDWARE
	#include "hardware/pio.h"
#endif

#define KBD_BUFFER_SIZE 45
static volatile uint8_t kbd_buffer[KBD_BUFFER_SIZE];
static volatile uint8_t head, tail;


static uint8_t dataKB[512];
kb_u_state kb_st_ps2;

//состояние шины данных спектрума для любого адресного состояния

uint8_t* zx_keyboard_state=dataKB;//&dataKB[0x100-];

uint64_t keyboard_state=0;
//состояние клавиатурной матрицы спектрума
//static
void zx_kb_decode(uint8_t* zx_kb_state)
{
	//memset(zx_keys_matrix,0,8);
	static uint64_t tmp_zx_kb_state64[32];
	uint8_t* tmp_zx_kb_state8=(uint8_t*)tmp_zx_kb_state64;
	
	uint8_t zx_keys_matrix[8];
	convert_kb_u_to_kb_zx(&kb_st_ps2,zx_keys_matrix);
	
	for(int i=0;i<256;i++)
	{
		uint8_t out8=0;
		uint8_t inx=i;
		for(int k=0;k<8;k++)
		{
			if (!(inx&1)) out8|=zx_keys_matrix[k];
			inx>>=1;
		}
		//if (out8!=0) G_PRINTF("i=%d\n",i);//test
		//if (zx_kb_state[i]!=(~out8)) zx_kb_state[i]=(~out8);
		tmp_zx_kb_state8[i]=(~out8);
	}
	//для быстрого копирования всего буфера
	
	
	uint64_t* dst_zx_kb_state64=(uint64_t*)zx_kb_state;
	uint64_t* src_zx_kb_state64=tmp_zx_kb_state64;
	for(int i=32;i--;)
	{
		*dst_zx_kb_state64++=*src_zx_kb_state64++;
	}
	
	
};



void translate_scancode(uint8_t code,bool is_press, bool is_e0,bool is_e1)
{
	if (is_e1){
		if (code==0x14) {if (is_press) kb_st_ps2.u[2]|=KB_U2_PAUSE_BREAK; else kb_st_ps2.u[2]&=~KB_U2_PAUSE_BREAK;}
		return;
	}
	
	if (!is_e0)
	switch (code)
	{
		//0
		case 0x1C: if (is_press) kb_st_ps2.u[0]|=KB_U0_A; else kb_st_ps2.u[0]&=~KB_U0_A; break;
		case 0x32: if (is_press) kb_st_ps2.u[0]|=KB_U0_B; else kb_st_ps2.u[0]&=~KB_U0_B; break;
		case 0x21: if (is_press) kb_st_ps2.u[0]|=KB_U0_C; else kb_st_ps2.u[0]&=~KB_U0_C; break;
		case 0x23: if (is_press) kb_st_ps2.u[0]|=KB_U0_D; else kb_st_ps2.u[0]&=~KB_U0_D; break;
		case 0x24: if (is_press) kb_st_ps2.u[0]|=KB_U0_E; else kb_st_ps2.u[0]&=~KB_U0_E; break;
		case 0x2B: if (is_press) kb_st_ps2.u[0]|=KB_U0_F; else kb_st_ps2.u[0]&=~KB_U0_F; break;
		case 0x34: if (is_press) kb_st_ps2.u[0]|=KB_U0_G; else kb_st_ps2.u[0]&=~KB_U0_G; break;
		case 0x33: if (is_press) kb_st_ps2.u[0]|=KB_U0_H; else kb_st_ps2.u[0]&=~KB_U0_H; break;
		case 0x43: if (is_press) kb_st_ps2.u[0]|=KB_U0_I; else kb_st_ps2.u[0]&=~KB_U0_I; break;
		case 0x3B: if (is_press) kb_st_ps2.u[0]|=KB_U0_J; else kb_st_ps2.u[0]&=~KB_U0_J; break;
		
		case 0x42: if (is_press) kb_st_ps2.u[0]|=KB_U0_K; else kb_st_ps2.u[0]&=~KB_U0_K; break;
		case 0x4B: if (is_press) kb_st_ps2.u[0]|=KB_U0_L; else kb_st_ps2.u[0]&=~KB_U0_L; break;
		case 0x3A: if (is_press) kb_st_ps2.u[0]|=KB_U0_M; else kb_st_ps2.u[0]&=~KB_U0_M; break;
		case 0x31: if (is_press) kb_st_ps2.u[0]|=KB_U0_N; else kb_st_ps2.u[0]&=~KB_U0_N; break;
		case 0x44: if (is_press) kb_st_ps2.u[0]|=KB_U0_O; else kb_st_ps2.u[0]&=~KB_U0_O; break;
		case 0x4D: if (is_press) kb_st_ps2.u[0]|=KB_U0_P; else kb_st_ps2.u[0]&=~KB_U0_P; break;
		case 0x15: if (is_press) kb_st_ps2.u[0]|=KB_U0_Q; else kb_st_ps2.u[0]&=~KB_U0_Q; break;
		case 0x2D: if (is_press) kb_st_ps2.u[0]|=KB_U0_R; else kb_st_ps2.u[0]&=~KB_U0_R; break;
		case 0x1B: if (is_press) kb_st_ps2.u[0]|=KB_U0_S; else kb_st_ps2.u[0]&=~KB_U0_S; break;
		case 0x2C: if (is_press) kb_st_ps2.u[0]|=KB_U0_T; else kb_st_ps2.u[0]&=~KB_U0_T; break;
		
		case 0x3C: if (is_press) kb_st_ps2.u[0]|=KB_U0_U; else kb_st_ps2.u[0]&=~KB_U0_U; break;
		case 0x2A: if (is_press) kb_st_ps2.u[0]|=KB_U0_V; else kb_st_ps2.u[0]&=~KB_U0_V; break;
		case 0x1D: if (is_press) kb_st_ps2.u[0]|=KB_U0_W; else kb_st_ps2.u[0]&=~KB_U0_W; break;
		case 0x22: if (is_press) kb_st_ps2.u[0]|=KB_U0_X; else kb_st_ps2.u[0]&=~KB_U0_X; break;
		case 0x35: if (is_press) kb_st_ps2.u[0]|=KB_U0_Y; else kb_st_ps2.u[0]&=~KB_U0_Y; break;
		case 0x1A: if (is_press) kb_st_ps2.u[0]|=KB_U0_Z; else kb_st_ps2.u[0]&=~KB_U0_Z; break;
		
		case 0x54: if (is_press) kb_st_ps2.u[0]|=KB_U0_LEFT_BR; else kb_st_ps2.u[0]&=~KB_U0_LEFT_BR; break;
		case 0x5B: if (is_press) kb_st_ps2.u[0]|=KB_U0_RIGHT_BR; else kb_st_ps2.u[0]&=~KB_U0_RIGHT_BR; break;
		case 0x4C: if (is_press) kb_st_ps2.u[0]|=KB_U0_SEMICOLON; else kb_st_ps2.u[0]&=~KB_U0_SEMICOLON; break;
		case 0x52: if (is_press) kb_st_ps2.u[0]|=KB_U0_QUOTE; else kb_st_ps2.u[0]&=~KB_U0_QUOTE; break;
		case 0x41: if (is_press) kb_st_ps2.u[0]|=KB_U0_COMMA; else kb_st_ps2.u[0]&=~KB_U0_COMMA; break;
		case 0x49: if (is_press) kb_st_ps2.u[0]|=KB_U0_PERIOD; else kb_st_ps2.u[0]&=~KB_U0_PERIOD; break;
		
		//1 -----------
		case 0x45: if (is_press) kb_st_ps2.u[1]|=KB_U1_0; else kb_st_ps2.u[1]&=~KB_U1_0; break;
		case 0x16: if (is_press) kb_st_ps2.u[1]|=KB_U1_1; else kb_st_ps2.u[1]&=~KB_U1_1; break;
		case 0x1E: if (is_press) kb_st_ps2.u[1]|=KB_U1_2; else kb_st_ps2.u[1]&=~KB_U1_2; break;
		case 0x26: if (is_press) kb_st_ps2.u[1]|=KB_U1_3; else kb_st_ps2.u[1]&=~KB_U1_3; break;
		case 0x25: if (is_press) kb_st_ps2.u[1]|=KB_U1_4; else kb_st_ps2.u[1]&=~KB_U1_4; break;
		case 0x2E: if (is_press) kb_st_ps2.u[1]|=KB_U1_5; else kb_st_ps2.u[1]&=~KB_U1_5; break;
		case 0x36: if (is_press) kb_st_ps2.u[1]|=KB_U1_6; else kb_st_ps2.u[1]&=~KB_U1_6; break;
		case 0x3D: if (is_press) kb_st_ps2.u[1]|=KB_U1_7; else kb_st_ps2.u[1]&=~KB_U1_7; break;
		case 0x3E: if (is_press) kb_st_ps2.u[1]|=KB_U1_8; else kb_st_ps2.u[1]&=~KB_U1_8; break;
		case 0x46: if (is_press) kb_st_ps2.u[1]|=KB_U1_9; else kb_st_ps2.u[1]&=~KB_U1_9; break;
		
		case 0x4E: if (is_press) kb_st_ps2.u[1]|=KB_U1_MINUS; else kb_st_ps2.u[1]&=~KB_U1_MINUS; break;
		case 0x55: if (is_press) kb_st_ps2.u[1]|=KB_U1_EQUALS; else kb_st_ps2.u[1]&=~KB_U1_EQUALS; break;
		case 0x5D: if (is_press) kb_st_ps2.u[1]|=KB_U1_BACKSLASH; else kb_st_ps2.u[1]&=~KB_U1_BACKSLASH; break;
		case 0x66: if (is_press) kb_st_ps2.u[1]|=KB_U1_BACK_SPACE; else kb_st_ps2.u[1]&=~KB_U1_BACK_SPACE; break;
		case 0x5A: if (is_press) kb_st_ps2.u[1]|=KB_U1_ENTER; else kb_st_ps2.u[1]&=~KB_U1_ENTER; break;
		case 0x4A: if (is_press) kb_st_ps2.u[1]|=KB_U1_SLASH; else kb_st_ps2.u[1]&=~KB_U1_SLASH; break;
		case 0x0E: if (is_press) kb_st_ps2.u[1]|=KB_U1_TILDE; else kb_st_ps2.u[1]&=~KB_U1_TILDE; break;
		case 0x0D: if (is_press) kb_st_ps2.u[1]|=KB_U1_TAB; else kb_st_ps2.u[1]&=~KB_U1_TAB; break;
		case 0x58: if (is_press) kb_st_ps2.u[1]|=KB_U1_CAPS_LOCK; else kb_st_ps2.u[1]&=~KB_U1_CAPS_LOCK; break;
		case 0x76: if (is_press) kb_st_ps2.u[1]|=KB_U1_ESC; else kb_st_ps2.u[1]&=~KB_U1_ESC; break;
		
		case 0x12: if (is_press) kb_st_ps2.u[1]|=KB_U1_L_SHIFT; else kb_st_ps2.u[1]&=~KB_U1_L_SHIFT; break;
		case 0x14: if (is_press) kb_st_ps2.u[1]|=KB_U1_L_CTRL; else kb_st_ps2.u[1]&=~KB_U1_L_CTRL; break;
		case 0x11: if (is_press) kb_st_ps2.u[1]|=KB_U1_L_ALT; else kb_st_ps2.u[1]&=~KB_U1_L_ALT; break;
		case 0x59: if (is_press) kb_st_ps2.u[1]|=KB_U1_R_SHIFT; else kb_st_ps2.u[1]&=~KB_U1_R_SHIFT; break;
		
		case 0x29: if (is_press) kb_st_ps2.u[1]|=KB_U1_SPACE; else kb_st_ps2.u[1]&=~KB_U1_SPACE; break;
		//2 -----------
		case 0x70: if (is_press) kb_st_ps2.u[2]|=KB_U2_NUM_0; else kb_st_ps2.u[2]&=~KB_U2_NUM_0; break;
		case 0x69: if (is_press) kb_st_ps2.u[2]|=KB_U2_NUM_1; else kb_st_ps2.u[2]&=~KB_U2_NUM_1; break;
		case 0x72: if (is_press) kb_st_ps2.u[2]|=KB_U2_NUM_2; else kb_st_ps2.u[2]&=~KB_U2_NUM_2; break;
		case 0x7A: if (is_press) kb_st_ps2.u[2]|=KB_U2_NUM_3; else kb_st_ps2.u[2]&=~KB_U2_NUM_3; break;
		case 0x6B: if (is_press) kb_st_ps2.u[2]|=KB_U2_NUM_4; else kb_st_ps2.u[2]&=~KB_U2_NUM_4; break;
		case 0x73: if (is_press) kb_st_ps2.u[2]|=KB_U2_NUM_5; else kb_st_ps2.u[2]&=~KB_U2_NUM_5; break;
		case 0x74: if (is_press) kb_st_ps2.u[2]|=KB_U2_NUM_6; else kb_st_ps2.u[2]&=~KB_U2_NUM_6; break;
		case 0x6C: if (is_press) kb_st_ps2.u[2]|=KB_U2_NUM_7; else kb_st_ps2.u[2]&=~KB_U2_NUM_7; break;
		case 0x75: if (is_press) kb_st_ps2.u[2]|=KB_U2_NUM_8; else kb_st_ps2.u[2]&=~KB_U2_NUM_8; break;
		case 0x7D: if (is_press) kb_st_ps2.u[2]|=KB_U2_NUM_9; else kb_st_ps2.u[2]&=~KB_U2_NUM_9; break;
		
		case 0x77: if (is_press) kb_st_ps2.u[2]|=KB_U2_NUM_LOCK; else kb_st_ps2.u[2]&=~KB_U2_NUM_LOCK; break;
		case 0x7C: if (is_press) kb_st_ps2.u[2]|=KB_U2_NUM_MULT; else kb_st_ps2.u[2]&=~KB_U2_NUM_MULT; break;
		case 0x7B: if (is_press) kb_st_ps2.u[2]|=KB_U2_NUM_MINUS; else kb_st_ps2.u[2]&=~KB_U2_NUM_MINUS; break;
		case 0x79: if (is_press) kb_st_ps2.u[2]|=KB_U2_NUM_PLUS; else kb_st_ps2.u[2]&=~KB_U2_NUM_PLUS; break;
		case 0x71: if (is_press) kb_st_ps2.u[2]|=KB_U2_NUM_PERIOD; else kb_st_ps2.u[2]&=~KB_U2_NUM_PERIOD; break;
		case 0x7E: if (is_press) kb_st_ps2.u[2]|=KB_U2_SCROLL_LOCK; else kb_st_ps2.u[2]&=~KB_U2_SCROLL_LOCK; break;
		//3 -----------
		case 0x05: if (is_press) kb_st_ps2.u[3]|=KB_U3_F1; else kb_st_ps2.u[3]&=~KB_U3_F1; break;
		case 0x06: if (is_press) kb_st_ps2.u[3]|=KB_U3_F2; else kb_st_ps2.u[3]&=~KB_U3_F2; break;
		case 0x04: if (is_press) kb_st_ps2.u[3]|=KB_U3_F3; else kb_st_ps2.u[3]&=~KB_U3_F3; break;
		case 0x0C: if (is_press) kb_st_ps2.u[3]|=KB_U3_F4; else kb_st_ps2.u[3]&=~KB_U3_F4; break;
		case 0x03: if (is_press) kb_st_ps2.u[3]|=KB_U3_F5; else kb_st_ps2.u[3]&=~KB_U3_F5; break;
		case 0x0B: if (is_press) kb_st_ps2.u[3]|=KB_U3_F6; else kb_st_ps2.u[3]&=~KB_U3_F6; break;
		case 0x83: if (is_press) kb_st_ps2.u[3]|=KB_U3_F7; else kb_st_ps2.u[3]&=~KB_U3_F7; break;
		case 0x0A: if (is_press) kb_st_ps2.u[3]|=KB_U3_F8; else kb_st_ps2.u[3]&=~KB_U3_F8; break;
		case 0x01: if (is_press) kb_st_ps2.u[3]|=KB_U3_F9; else kb_st_ps2.u[3]&=~KB_U3_F9; break;
		case 0x09: if (is_press) kb_st_ps2.u[3]|=KB_U3_F10; else kb_st_ps2.u[3]&=~KB_U3_F10; break;
		
		case 0x78: if (is_press) kb_st_ps2.u[3]|=KB_U3_F11; else kb_st_ps2.u[3]&=~KB_U3_F11; break;
		case 0x07: if (is_press) kb_st_ps2.u[3]|=KB_U3_F12; else kb_st_ps2.u[3]&=~KB_U3_F12; break;
		
		
		
		default:
		break;
	}
	if (is_e0)
	switch (code)
	{
		//1----------------
		case 0x1F: if (is_press) kb_st_ps2.u[1]|=KB_U1_L_WIN; else kb_st_ps2.u[1]&=~KB_U1_L_WIN; break;
		case 0x14: if (is_press) kb_st_ps2.u[1]|=KB_U1_R_CTRL; else kb_st_ps2.u[1]&=~KB_U1_R_CTRL; break;
		case 0x11: if (is_press) kb_st_ps2.u[1]|=KB_U1_R_ALT; else kb_st_ps2.u[1]&=~KB_U1_R_ALT; break;
		case 0x27: if (is_press) kb_st_ps2.u[1]|=KB_U1_R_WIN; else kb_st_ps2.u[1]&=~KB_U1_R_WIN; break;
		case 0x2F: if (is_press) kb_st_ps2.u[1]|=KB_U1_MENU; else kb_st_ps2.u[1]&=~KB_U1_MENU; break;
		//2------------------
		//для принт скрин обработаем только 1 код
		case 0x12: if (is_press) kb_st_ps2.u[2]|=KB_U2_PRT_SCR; else kb_st_ps2.u[2]&=~KB_U2_PRT_SCR; break;
		
		
		case 0x4A: if (is_press) kb_st_ps2.u[2]|=KB_U2_NUM_SLASH; else kb_st_ps2.u[2]&=~KB_U2_NUM_SLASH; break;
		case 0x5A: if (is_press) kb_st_ps2.u[2]|=KB_U2_NUM_ENTER; else kb_st_ps2.u[2]&=~KB_U2_NUM_ENTER; break;
		case 0x75: if (is_press) kb_st_ps2.u[2]|=KB_U2_UP; else kb_st_ps2.u[2]&=~KB_U2_UP; break;
		case 0x72: if (is_press) kb_st_ps2.u[2]|=KB_U2_DOWN; else kb_st_ps2.u[2]&=~KB_U2_DOWN; break;
		case 0x74: if (is_press) kb_st_ps2.u[2]|=KB_U2_RIGHT; else kb_st_ps2.u[2]&=~KB_U2_RIGHT; break;
		case 0x6B: if (is_press) kb_st_ps2.u[2]|=KB_U2_LEFT; else kb_st_ps2.u[2]&=~KB_U2_LEFT; break;
		case 0x71: if (is_press) kb_st_ps2.u[2]|=KB_U2_DELETE; else kb_st_ps2.u[2]&=~KB_U2_DELETE; break;
		case 0x69: if (is_press) kb_st_ps2.u[2]|=KB_U2_END; else kb_st_ps2.u[2]&=~KB_U2_END; break;
		case 0x7A: if (is_press) kb_st_ps2.u[2]|=KB_U2_PAGE_DOWN; else kb_st_ps2.u[2]&=~KB_U2_PAGE_DOWN; break;
		case 0x7D: if (is_press) kb_st_ps2.u[2]|=KB_U2_PAGE_UP; else kb_st_ps2.u[2]&=~KB_U2_PAGE_UP; break;
		
		case 0x6C: if (is_press) kb_st_ps2.u[2]|=KB_U2_HOME; else kb_st_ps2.u[2]&=~KB_U2_HOME; break;
		case 0x70: if (is_press) kb_st_ps2.u[2]|=KB_U2_INSERT; else kb_st_ps2.u[2]&=~KB_U2_INSERT; break;
		
		
	}
	
	
	
	
}



void  __not_in_flash_func(ps2_proc) (uint8_t val){
	
	
	
	static uint8_t bitcount=0;
	static uint8_t incoming=0;
	static uint32_t prev_ms=0;
	uint32_t now_ms;
	uint8_t n;
	val=val?1:0;
	
	// now_ms = esp_timer_get_time()/1000;
	// if (now_ms - prev_ms > 250) {
	//   bitcount = 0;
	//   incoming = 0;
	// }
	//prev_ms = now_ms;
	
	
	n = bitcount - 1;
	if (n <= 7) {
		incoming |= (val << n);
	}
	bitcount++;
	if (bitcount == 11) {
		uint8_t i = head + 1;
		if (i >= KBD_BUFFER_SIZE) i = 0;
		if (i != tail) {
			kbd_buffer[i] = incoming;
			head = i;
		}
		bitcount = 0;
		incoming = 0;
	}
	
}

//static inline uint8_t get_scan_code(void)

uint8_t __not_in_flash_func(get_scan_code)(void)

{
	uint8_t c, i;
	
	i = tail;
	if (i == head) return 0;
	i++;
	if (i >= KBD_BUFFER_SIZE) i = 0;
	c = kbd_buffer[i];
	tail = i;
	return c;
}

bool decode_PS2()
{
	static bool is_e0=false;
	static bool is_e1=false;
	static bool is_f0=false;
	static char test_str[128];   
	
	uint8_t scancode=get_scan_code();
	if (scancode==0xe0) {is_e0=true;return false;}
	if (scancode==0xe1) {is_e1=true;return false;}
	if (scancode==0xf0) {is_f0=true;return false;}  
	if (scancode)
	{
		//сканкод 
		
		//получение универсальных кодов из сканкодов PS/2
		translate_scancode(scancode,!is_f0,is_e0,is_e1);
		is_e0=false;
		if (is_f0) is_e1=false;
		is_f0=false;
		//test
		//  keys_to_str(test_str,' ',kb_st_ps2);
		//  G_PRINTF_DEBUG("is_e0=%d, is_f0=%d, code=0x%02x\n",is_e0,is_f0,scancode);
		//  G_PRINTF_DEBUG(test_str);
		
		
		
		//преобразование из универсальных сканкодов в матрицу бытрого преобразования кодов для zx клавиатуры
		//zx_kb_decode(zx_keyboard_state);
		
		return true;//произошли изменения
		
	}
	return false;
}



//аппаратная работа с интерфейсом PS/2
//через захват пинов с помощью PIO
int dma_chan;

#define SIZE_LINE_CAPTURE (100)
//количество буферов ДМА можно увеличить, если будет пропуск, но не сильно, иначе будет большая задержка
#define N_DMA_BUF_CAPTURE 1
#define SIZE_DMA_BUF_CAPTURE (SIZE_LINE_CAPTURE*N_DMA_BUF_CAPTURE)
static uint8_t DMA_BUF_CAP[4][SIZE_DMA_BUF_CAPTURE];
static uint8_t* DMA_BUF_ADDR_CAP[4];


void __not_in_flash_func(dma_handler_capture()) {
	
	static uint32_t inx_buf_dma;  
	
	dma_hw->ints1 = 1u << dma_chan;
	dma_channel_set_read_addr(dma_chan,&DMA_BUF_ADDR_CAP[inx_buf_dma&3], false);
	
	inx_buf_dma++;
	uint8_t* buf8=DMA_BUF_CAP[inx_buf_dma&3];
	
	//работа с клавиатурой PS/2
	uint8_t* kb_line_data=buf8;
	static uint8_t old_state_PS2_CLK;
	for(int k=0;k<SIZE_DMA_BUF_CAPTURE;k++)
	{   
		//работа с данными PS/2
		const uint8_t mask_PS2_CLK=0x01;
		const uint8_t mask_PS2_DATA=0x02;
		uint8_t data8=*kb_line_data;
		uint8_t state_PS2_CLK=data8&mask_PS2_CLK;
		if (old_state_PS2_CLK^state_PS2_CLK) //поменялось состояние PS2_CLK
		{
			old_state_PS2_CLK=state_PS2_CLK;
			if (state_PS2_CLK==0) ps2_proc(data8&mask_PS2_DATA);//считываем данные по 0 состоянию CLK PS2
		} 
		kb_line_data++;
	}
	//
	
};

void start_PS2_capture(){   
	
	int sm=SM_in_ps2;
	int pin=beginPS2_PIN;
	
	uint offset = pio_add_program(PIO_p1, &pio_program1);
	
	
	pio_sm_config c = pio_get_default_sm_config();
	sm_config_set_wrap(&c, offset + pio_program1_wrap_target, offset + pio_program1_wrap);
	
	
	sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);
	
	sm_config_set_in_shift(&c, false, true, 32);
	sm_config_set_in_pins(&c, pin);
	
	
	
	pio_sm_init(PIO_p1, sm, offset+3, &c);
	
	pio_sm_set_enabled(PIO_p1, sm, true);
	
	float fdiv=clock_get_hz(clk_sys)/200000;//частота опроса ps/2 X4 
	PIO_p1->sm[sm].clkdiv=(uint32_t) (fdiv * (1 << 16)); //делитель для конкретной sm
	
	PIO_p1->txf[sm]=sm;//выбор подпрограммы захвата по номеру SM
	
	
	DMA_BUF_ADDR_CAP[0]=&DMA_BUF_CAP[0][0];
	DMA_BUF_ADDR_CAP[1]=&DMA_BUF_CAP[1][0];
	DMA_BUF_ADDR_CAP[2]=&DMA_BUF_CAP[2][0];
	DMA_BUF_ADDR_CAP[3]=&DMA_BUF_CAP[3][0];
	
	
	
	
	
	int dma_chan0 = dma_claim_unused_channel(true);
	dma_chan  = dma_claim_unused_channel(true);
	
	dma_channel_config c0 = dma_channel_get_default_config(dma_chan0);
	channel_config_set_transfer_data_size(&c0, DMA_SIZE_8);
	
	channel_config_set_read_increment(&c0, false);
	channel_config_set_write_increment(&c0, true);
	
	uint dreq=DREQ_PIO1_RX0+sm;
	if (PIO_p1==pio0) dreq=DREQ_PIO0_RX0+sm;
	
	
	
	
	channel_config_set_dreq(&c0, dreq);
	channel_config_set_chain_to(&c0, dma_chan);
	
	dma_channel_configure(
		dma_chan0,
		&c0,
		&DMA_BUF_CAP[0][0], // Write address 
		&PIO_p1->rxf[sm],			 //  read address
		SIZE_DMA_BUF_CAPTURE, // 
		false			 // Don't start yet
	);
	
	dma_channel_config c1 = dma_channel_get_default_config(dma_chan);
	channel_config_set_transfer_data_size(&c1, DMA_SIZE_32);
	
	channel_config_set_read_increment(&c1, false);
	channel_config_set_write_increment(&c1, false);
	channel_config_set_chain_to(&c1, dma_chan0);						 // chain to other channel
	
	
	dma_channel_configure(
		dma_chan,
		&c1,
		&dma_hw->ch[dma_chan0].write_addr, // Write address 
		&DMA_BUF_ADDR_CAP[0],			 // read address 
		1, // 
		false			 // Don't start yet
	);
	
	
	
	dma_channel_set_irq1_enabled(dma_chan, true);
	
	// Configure the processor to run dma_handler() when DMA IRQ 0 is asserted
	irq_set_exclusive_handler(DMA_IRQ_1, dma_handler_capture);
	irq_set_enabled(DMA_IRQ_1, true);
	
	
	
	dma_start_channel_mask((1u << dma_chan)) ;
	
	G_PRINTF_INFO("init PS/2\n");
	
	
};
