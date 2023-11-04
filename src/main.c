#define PICO_FLASH_SPI_CLKDIV 4
// #define PICO_FLASH_SIZE_BYTES (4 * 1024 * 1024)
//PICO_FLASH_SPI_CLKDIV

#define FW_VERSION "v0.30"
#define FW_AUTHOR "tecnocat"

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/flash.h"
#include <hardware/sync.h>
#include <hardware/irq.h>
#include "hardware/watchdog.h"
#include "hardware/clocks.h"
#include "hardware/structs/systick.h"
#include "hardware/pwm.h"
#include "hardware/vreg.h"
#include "screen_util.h"
#include "util_sd.h"
#include "util_z80.h"
#include "util_sna.h"
#include "util_tap.h"
#include "string.h"
#include "small_logo.h"
#include "mur_logo.h"
#include "mur_logo2.h"
#include "utf_handle.h"
#include "help.h"
////////////////////////////////////////////////
//tr-dos
#include "util_trd.h"
#include "trdos.h"
///////////////////////////////////////////////
#define AY_MODE
#include "util_tap.h"
//extern bool z80_gen_nmi_from_main;//  = false;
extern bool im_z80_stop;
extern bool im_ready_loading;
char temp_msg[60]; // Буфер для вывода строк
#define SHOW_SCREEN_DELAY 50 //в милисекундах

extern size_t tapeFileSize;
extern uint32_t tapeTotByteCount;

#include "g_config.h"
#include "VGA.h"
#include "ps2.h"
#include "PIO_program1.h"

#include "zx_emu/zx_machine.h"
#include "zx_emu/aySoundSoft.h"


void software_reset(){
	watchdog_enable(1, 1);
	while(1);
}

//функция ввода загрузки спектрума
uint8_t valLoad=0;

#define PIN_ZX_LOAD (22)
//функции вывода звука спектрума
#define ZX_AY_PWM_PIN0 (26)
#define ZX_AY_PWM_PIN1 (27)
#define ZX_BEEP_PIN (28)
uint8_t AY_DATA_R=0;
uint8_t AY_DATA_L=0;

bool b_beep;
bool b_save;


bool __not_in_flash_func(hw_zx_get_bit_LOAD)(){
    bool out;
    if (tap_loader_active){ // Переменная определена в util_tap.h
        if (TapeStatus==TAPE_LOADING){
            out = TAP_Read(); // Переменная определена в util_tap.h
		}	
	} else {
        out = gpio_get(PIN_ZX_LOAD);
	};
    valLoad=out*5; // valLoad=out*10;
    // pwm_set_gpio_level(ZX_AY_PWM_PIN0,AY_DATA_L+valLoad + b_beep*20 + b_save*10);       //  pwm_set_gpio_level(ZX_AY_PWM_PIN0,out);
    // pwm_set_gpio_level(ZX_AY_PWM_PIN1,AY_DATA_L+valLoad + b_beep*20 + b_save*10);       //  pwm_set_gpio_level(ZX_AY_PWM_PIN1,out);
    //gpio_put(ZX_RGB_PIN,out);
    return out;
};


void* ZXThread(){
	zx_machine_init();
	zx_machine_main_loop_start();
	G_PRINTF_INFO("END spectrum emulation\n");
	return NULL;
}

void  inInit(uint gpio){
	gpio_init(gpio);
	gpio_set_dir(gpio,GPIO_IN);
	gpio_pull_up(gpio);
	
}

#define TST_PIN (29)

// uint8_t valBEEP=0;
// uint8_t valSAVE=0;

// void __not_in_flash_func(hw_zx_set_save_out)(bool val) {b_save=val;};//;gpio_put(ZX_BEEP_PIN,b_beep^b_save);};
// void __not_in_flash_func(hw_zx_set_snd_out)(bool val) { 
// 	if(val!=b_beep){
// 		b_beep=val;
// 		gpio_put(ZX_BEEP_PIN,5*b_beep);
// 	} 
// };

uint8_t beep_data_old;
uint8_t beep_data;

void __not_in_flash_func(hw_zx_set_snd_out)(bool val){
	beep_data=(beep_data&0b01)|(val<<1);
};
void __not_in_flash_func(hw_zx_set_save_out)(bool val){
	static bool out;
	beep_data=(beep_data&0b10)|(val<<0);
	out^=(beep_data==beep_data_old)?0:1;
	gpio_put(ZX_BEEP_PIN,out);
	beep_data_old=beep_data;
	
};


void PWM_init_pin(uint pinN){
	gpio_set_function(pinN, GPIO_FUNC_PWM);
	uint slice_num = pwm_gpio_to_slice_num(pinN);
	
	pwm_config c_pwm=pwm_get_default_config();
	pwm_config_set_clkdiv(&c_pwm,1.0);
	pwm_config_set_wrap(&c_pwm,255);//MAX PWM value
	pwm_init(slice_num,&c_pwm,true);
}

#ifdef AY_MODE
	bool __not_in_flash_func(AY_timer_callback)(repeating_timer_t *rt){
        static uint inx; 
        static uint8_t outL=0;  
        static uint8_t outR=0; 
        pwm_set_gpio_level(ZX_AY_PWM_PIN0,outR); // Право
        pwm_set_gpio_level(ZX_AY_PWM_PIN1,outL); // Лево
        uint8_t* AY_data=get_AY_Out(5);
		
        outL=2*(AY_data[0]+AY_data[1])+(b_save * 0); //+(b_beep*10);//+(b_save*10);//+valBEEP+valSAVE;
        outR=2*(AY_data[2]+AY_data[1])+(valLoad * 4);       //+(b_beep*10);//+(b_save*10);//+valBEEP+valSAVE;
		
		//outL=2*(AY_data[0]+AY_data[1])+(b_beep*10)+(b_save * 10);
		//outR=2*(AY_data[2]+AY_data[1])+(b_beep*10)+valLoad;
		
        return true;
	}
	
#else

    void __not_in_flash_func(hw_zx_set_snd_out)(bool val) { b_beep=val;};
	
    bool __not_in_flash_func(AY_timer_callback)(repeating_timer_t *rt){
        static uint inx;
		
        pwm_set_gpio_level(ZX_AY_PWM_PIN0,AY_DATA_L+valLoad + b_beep*20 + b_save*20);       //  pwm_set_gpio_level(ZX_AY_PWM_PIN0,out);
        pwm_set_gpio_level(ZX_AY_PWM_PIN1,AY_DATA_R+valLoad + b_beep*20 + b_save*20);       //  pwm_set_gpio_level(ZX_AY_PWM_PIN1,out);
		
        uint8_t* AY_data=get_AY_Out(5);
		
        AY_DATA_L=2*AY_data[0]+AY_data[1];
        AY_DATA_R=2*AY_data[2]+AY_data[1];
		
        return true;
	}
    
#endif


bool zx_flash_callback(repeating_timer_t *rt) {zx_machine_flashATTR();};

//Joy joy1 = {2, 5, 4, 0, 0, 0};

#define MAX_JOY_MODE 5

uint8_t joyMode=0;

const char __in_flash() *joy_text[MAX_JOY_MODE]={
	"    Ext Joystick     ",
	"  Kempston Joystick  ",
	" Interface2 Joystick ",
	"   Cursor Joystick   ",
	" ---===[NONE]===---  "
};

#define D_JOY_DATA_PIN (16)
#define D_JOY_CLK_PIN (14)
#define D_JOY_LATCH_PIN (15)
volatile int us_val=0;

void d_sleep_us(uint us){
	for(uint i=0;i<us;i++){
		for(int j=0;j<25;j++){
			us_val++;
		}
	}
}

uint8_t d_joy_get_data(){
	uint8_t data;
	gpio_put(D_JOY_LATCH_PIN,1);
	//gpio_put(D_JOY_CLK_PIN,1);
	d_sleep_us(12);
	gpio_put(D_JOY_LATCH_PIN,0);
	d_sleep_us(6);
	
	for(int i=0;i<8;i++){   
		
		gpio_put(D_JOY_CLK_PIN,0);  
		d_sleep_us(10);
		data<<=1;
		data|=gpio_get(D_JOY_DATA_PIN);
		d_sleep_us(10);
		
		
		gpio_put(D_JOY_CLK_PIN,1); 
		d_sleep_us(10);
		
	}
	return data;
};

void d_joy_init(){
	gpio_init(D_JOY_CLK_PIN);
	gpio_set_dir(D_JOY_CLK_PIN,GPIO_OUT);
	gpio_init(D_JOY_LATCH_PIN);
	gpio_set_dir(D_JOY_LATCH_PIN,GPIO_OUT);
	
	gpio_init(D_JOY_DATA_PIN);
	gpio_set_dir(D_JOY_DATA_PIN,GPIO_IN);
	gpio_pull_up(D_JOY_DATA_PIN);
	//gpio_pull_down(D_JOY_DATA_PIN);
	gpio_put(D_JOY_LATCH_PIN,0);	
}


/*-------Graphics--------*/
void draw_logo_header(){
	//draw_rect(7,6,SCREEN_W-15,FONT_H,COLOR_FULLSCREEN,true);
	//draw_rect(7,6,SCREEN_W-195,FONT_H,COLOR_BORDER,true);
/* 	for(uint8_t y=0;y<8;y++){ //Рисуем верхнее лого
		for(uint8_t x=0;x<43;x++){
			uint8_t pixel = small_logo[x+(y*43)];
			if(pixel>0) draw_pixel(189+x,7+y,pixel);
		}
	} */
	//draw_text_len(FONT_W*29,FONT_H-1,"MURMULATOR",COLOR_TEXT,COLOR_FULLSCREEN,10);
//	draw_text_len(FONT_W*29,FONT_H-1,  "TEST v0.00",CL_LT_RED,COLOR_BORDER,10);
	draw_text(FONT_W*29,FONT_H,  "TEST v0.00",CL_LT_RED,COLOR_BORDER);
}

void draw_mur_logo(){
	//draw_rect(16+FONT_W*14,15,184,192,COLOR_BORDER,true);	
	draw_text_len(186,70,"NO SIGNAL",COLOR_BACKGOUND,COLOR_TEXT,9);
	for(uint8_t y=0;y<83;y++){
		for(uint8_t x=0;x<84;x++){
			uint8_t pixel = mur_logo[x+(y*84)];
			draw_pixel(180+x,90+y,pixel);
		}
	}
}

void draw_mur_logo_big(uint8_t xPos,uint8_t yPos,uint8_t help){ //x-155 y-60
	for(uint8_t y=0;y<110;y++){
		for(uint8_t x=0;x<138;x++){
			uint8_t pixel = mur_logo2[x+(y*138)];
			if (pixel<0xFF)	draw_pixel(xPos+x,yPos+y,pixel);
		}
	}
	if(help==1){
		draw_text_len(xPos+FONT_W+2,yPos+110+FONT_H*2,"   F1 - HELP   ",CL_GREEN,CL_BLACK,15);
		draw_text_len(xPos+FONT_W+2,yPos+110+FONT_H*3,"WIN,HOME-RETURN",CL_GREEN,CL_BLACK,15);	
	} else 
	if(help==2){
		draw_text_len(xPos+FONT_W+2,yPos+110+FONT_H*2," WIN,HOME-MENU ",CL_GREEN,CL_BLACK,15);	
		draw_text_len(xPos+FONT_W+2,yPos+110+FONT_H*3,"   F1 - HELP   ",CL_GREEN,CL_BLACK,15);
	}


draw_text(xPos-12,yPos+110+FONT_H*5,"TEST VERSION  TR-DOS",CL_RED,CL_BLACK);	

}

void draw_help_text(){
	for(uint8_t y=0;y<26;y++){
		memset(temp_msg,0,sizeof(temp_msg));
		//printf("%s\n",help_text[y]);
		if (convert_utf8_to_windows1251(help_text[y], temp_msg, strlen(help_text[y]))>0){
			draw_text_len(11,20+(y*8),temp_msg,CL_BLACK,CL_WHITE,37);
		};
		//printf("%s\n",temp_msg);
		memset(temp_msg, 0, sizeof(temp_msg));
	}
}

void MessageBox(char *text,char *text1,uint8_t colorFG,uint8_t colorBG,uint8_t over_emul){
	uint8_t max_len= strlen(text)>strlen(text1)?strlen(text):strlen(text1);
	uint8_t left_x =(SCREEN_W/2)-((max_len/2)*FONT_W);
	uint8_t left_y = strlen(text1)==0 ? (SCREEN_H/2)-(FONT_H/2):(SCREEN_H/2)-FONT_H;
	uint8_t height = strlen(text1)>0 ? FONT_H*2+5:FONT_H+5;
	if(over_emul) zx_machine_enable_vbuf(false);
	draw_rect(left_x-3,left_y-3,(max_len*FONT_W)+5,height+2,colorBG,true);
	draw_rect(left_x-2,left_y-2,(max_len*FONT_W)+3,height,colorFG,false);
	draw_text(left_x,left_y+1,text,colorFG,colorBG);
	if (strlen(text1)>0) draw_text(left_x,left_y+FONT_H+1,text1,colorFG,colorBG);
	//printf("X:%d,Y:%d\n",left_x,left_y);
	switch (over_emul){
	case 1:
		g_delay_ms(3000);
		break;
	case 2:
		g_delay_ms(750);
		break;
	case 3:
		g_delay_ms(250);
		break;
	case 4:
		g_delay_ms(1000);
		break;
	default:
		break;
	}
}

void draw_main_window(){
	draw_rect(0,0,SCREEN_W,SCREEN_H,COLOR_FULLSCREEN,true);//Заливаем экран 
	draw_rect(7,6,SCREEN_W-15,SCREEN_H-14,COLOR_BORDER,true);//Основная рамка
	draw_rect(8,8+FONT_H,SCREEN_W-17,SCREEN_H-25,COLOR_BACKGOUND,true);//Фон главного окна
	draw_logo_header();
	memset(temp_msg,0,sizeof(temp_msg));
	sprintf(temp_msg,"%s %s",FW_VERSION,FW_AUTHOR);
	draw_text_len(FONT_W,FONT_H-1,temp_msg,CL_GRAY,COLOR_BORDER,15);
	memset(temp_msg, 0, sizeof(temp_msg));
}

void draw_file_window(){
	draw_rect(17+FONT_W*14,208,182,22,COLOR_BACKGOUND,true); //Фон отображения скринов
	draw_rect(8+FONT_W*14,15,8,SCREEN_H-23,COLOR_BORDER,false);  //Рамка полосы прокрутки
	
	//draw_rect(7,SCREEN_H-FONT_H-8,9+FONT_W*14,FONT_H+1,COLOR_BORDER,false);//панель подсказок под файлами
	//draw_text_len(8,SCREEN_H-FONT_H-7,"F1-HLP,HOME-RET",COLOR_TEXT,COLOR_BACKGOUND,15);//get_file_from_dir("0:/z80",i)
	
	//*draw_rect(17+FONT_W*14,16,182,SCREEN_H-25,COLOR_BACKGOUND,true);  
	//draw_rect(17+FONT_W*14+11,16+11,160,120,0x0,true);
	//*draw_rect(17+FONT_W*14,16,182,192,COLOR_BACKGOUND,true);
	//*draw_line(17+FONT_W*14,209,17+FONT_W*37,209,COLOR_PIC_BG);
	
}
/*-------Graphics--------*/

uint32_t last_action = 0;
bool showEmpySlots = false;


int main(void){  
	vreg_set_voltage(VREG_VOLTAGE_1_20);
	sleep_ms(100);
	//set_sys_clock_khz(300000, true);
	set_sys_clock_khz(252000, false);
	//set_sys_clock_khz(264000, false);
	sleep_ms(10);
	
	
	stdio_init_all();
	g_delay_ms(100);
	
	//g_delay_ms(2000);
	
	G_PRINTF("MurMulator %s by %s \n\n",FW_VERSION,FW_AUTHOR);
	G_PRINTF("Main Program Start!!!\n");
	G_PRINTF("CPU clock=%ld Hz\n",clock_get_hz(clk_sys));
	
	//пин ввода звука
	inInit(PIN_ZX_LOAD);
	//пины вывода звука
	PWM_init_pin(ZX_AY_PWM_PIN0);
	PWM_init_pin(ZX_AY_PWM_PIN1);
	repeating_timer_t timer;
	
	gpio_init(TST_PIN);
	gpio_set_dir(TST_PIN,GPIO_OUT);
	
	
	gpio_init(ZX_BEEP_PIN);
	gpio_set_dir(ZX_BEEP_PIN,GPIO_OUT);
	
	int hz = 44100;	//44000 //44100 //96000 //22050
	// negative timeout means exact delay (rather than delay between callbacks)
	if (!add_repeating_timer_us(-1000000 / hz, AY_timer_callback, NULL, &timer)) {
		G_PRINTF_ERROR("Failed to add timer\n");
		return 1;
	}
	
	repeating_timer_t zx_flash_timer;
	hz=2;
	if (!add_repeating_timer_us(-1000000 / hz, zx_flash_callback, NULL, &zx_flash_timer)) {
		G_PRINTF_ERROR("Failed to add zx flash timer\n");
		return 1;
	}

	d_joy_init();
	
	g_delay_ms(100);
	
	startVGA();
	g_delay_ms(100);
	start_PS2_capture();
	//multicore_launch_core1( start_PS2_capture);
	
	//ZXThread();
	multicore_launch_core1(ZXThread);
	g_delay_ms(50);
	
	
	ZX_Input_t zx_input;
	// memset(&zx_input,0,sizeof(ZX_Input_t));
	//kb_u_state kb_st;
	// memset(&kb_st,0,sizeof(kb_st));
	
	//	d_joy_init();
	
	//	Joy_init(&joy1);
	//   Joy_init(&joy2);
	
	uint8_t data_joy=0;
	uint inx=0;		
	uint8_t new_data_joy;
	// uint8_t new_start_button;
	// uint8_t start_button=0;
	// uint8_t start_key=1;
	
	// uint8_t new_select_button;
	// uint8_t select_button=0;
	// // uint8_t select_key=1;
	
	// uint8_t new_B_button;
	// uint8_t B_button=0;
	// uint8_t B_key=1;
	
	// uint8_t new_left_button;
	// uint8_t left_button=0;
	
	// uint8_t new_rigth_button;
	// uint8_t rigth_button=0;
	
	// uint8_t new_up_button;
	// uint8_t up_button=0;
	
	// uint8_t new_down_button;
	// uint8_t down_button=0;
	
	// uint8_t new_fire_button;
	// uint8_t fire_button=0;
	
	// uint8_t joy_mode=0;
	
	// char joy_mode_str[4][12] = {"kempston","sinclair1","sinclair2","cursor"};
	// #define JOY_KEMPSTON_MODE 0
	// #define JOY_SINCLAIR1_MODE 1
	// #define JOY_SINCLAIR2_MODE 2
	// #define JOY_CURSOR_MODE 3
	
	convert_kb_u_to_kb_zx(&kb_st_ps2,zx_input.kb_data);
	
	
	inInit(PIN_ZX_LOAD);
	
	printf("init FileSystem\n");
    int fs=vfs_init();
    printf("init FS:%d\n",fs);
    /*if ((fs!=-FR_NOT_ENABLED)&&(fs!=FR_OK)) software_reset();
    printf("init FS-OK\n");*/
	
	
	
	init_screen(g_gbuf,SCREEN_W,SCREEN_H);
	
	
	
	//инициализация переменных для меню
	bool read_dir = true;
	int shift_file_index=0;
	int cur_dir_index=0;
	int cur_file_index=0;
	int cur_file_index_old=0;
	int N_files=0;
	//uint64_t current_time = 0;
	char icon[2];
	uint8_t sound_reg_pause[18];
	
	int tap_block_percent = 0;
	
	bool is_menu_mode=false;
	bool old_key_menu_state=false;
	bool key_menu_state=false;
	bool is_new_screen=false;
	
	bool is_help_mode=false;
	bool old_key_help_state=false;
	bool key_help_state=false;
	bool help_mode_draw=false;
	
	static bool is_emulation_mode=true;
	
	bool is_pause_mode=false;
	bool old_key_pause_state=false;
	bool key_pause_state=false;
	uint8_t paused=0;
	tap_loader_active=false;
	char save_file_name_image[25];
	FIL f;
	int fr =-1;


	//boot logo
	zx_machine_enable_vbuf(false);
	draw_rect(0,0,SCREEN_W,SCREEN_H,COLOR_FULLSCREEN,true);//Заливаем экран 
	draw_mur_logo_big(SCREEN_W/2-69,SCREEN_H/2-75,2);

	printf("starting main loop \n");


    InitTRDOS();


	while(1){
		//current_time = get_absolute_time();
		if (decode_PS2()){
			if (((kb_st_ps2.u[1]&KB_U1_L_SHIFT)||(kb_st_ps2.u[1]&KB_U1_R_SHIFT))&&((kb_st_ps2.u[1]&KB_U1_L_ALT)||(kb_st_ps2.u[1]&KB_U1_R_ALT))&&(kb_st_ps2.u[2]&KB_U2_DELETE)){ //((kb_st_ps2.u[1]&KB_U1_L_CTRL)||(kb_st_ps2.u[1]&KB_U1_R_CTRL))&&
				//G_PRINTF_INFO("restart\n");
				software_reset();
			}

			//g_delay_ms(10);
			//menu
			//кнопка перехода в меню по HOME
			key_menu_state=(kb_st_ps2.u[2]&KB_U2_HOME)|(kb_st_ps2.u[1]&KB_U1_L_WIN)|(kb_st_ps2.u[1]&KB_U1_R_WIN);
			//printf("key_menu_state:%d\told_key_menu_state:%d\n",key_menu_state,old_key_menu_state);
			if (is_help_mode==false){
				if(key_menu_state&!old_key_menu_state) {is_menu_mode^=1;is_new_screen=true;} else {is_new_screen=false;}
				old_key_menu_state=key_menu_state;
			}
			
			//кнопка вызова помощи по F1

			if((is_help_mode)&&(help_mode_draw==false)){ //выход из помощи любой кнопкой
				if((kb_st_ps2.u[0]>0)||(kb_st_ps2.u[1]>0)||(kb_st_ps2.u[2]>0)||(kb_st_ps2.u[3]>0)){
					is_help_mode=false;
					is_new_screen=true;
					memset(kb_st_ps2.u,0,sizeof(kb_st_ps2.u));
					//g_delay_ms(200);
					continue;
				} 
			}

			if(!((kb_st_ps2.u[1]&KB_U1_L_SHIFT || kb_st_ps2.u[1]&KB_U1_R_SHIFT)||(kb_st_ps2.u[1]&KB_U1_L_CTRL || kb_st_ps2.u[1]&KB_U1_R_CTRL))){
				key_help_state=(kb_st_ps2.u[3]&KB_U3_F1);
			}
			if(key_help_state&!old_key_help_state) {is_help_mode^=1;help_mode_draw=true;} else {help_mode_draw=false;}
			old_key_help_state=key_help_state;
		
			if((key_help_state)&&(old_key_help_state)&&(is_menu_mode)){is_new_screen=true;}
			

			
			//кнопка остановки по PAUSE
			key_pause_state=(kb_st_ps2.u[2]&KB_U2_PAUSE_BREAK);
			if(key_pause_state&!old_key_pause_state) {is_pause_mode^=1;};
			old_key_pause_state=key_pause_state;
			
			is_emulation_mode = !(is_menu_mode|is_help_mode);
			
			//printf("u[0]:%d\tu[1]:%d\tu[2]:%d\tu[3]:%d\n",kb_st_ps2.u[0],kb_st_ps2.u[1],kb_st_ps2.u[2],kb_st_ps2.u[3]);
			//printf("is_menu_mode:%d is_help_mode:%d is_emulation_mode:%d is_pause_mode:%d\n",is_menu_mode,is_help_mode,is_emulation_mode,is_pause_mode);
			//printf("key_menu_state:%d\told_key_menu_state:%d\tis_menu_mode:%d\tis_new_screen:%d\n",key_menu_state,old_key_menu_state,is_menu_mode,is_new_screen);
			//printf("key_help_state:%d\told_key_help_state:%d\tis_help_mode:%d\thelp_mode_draw:%d\n",key_help_state,old_key_help_state,is_help_mode,help_mode_draw);
			//printf("key_pause_state:%d\told_key_pause_state:%d\tis_pause_mode:%d\tpaused:%d\n",key_pause_state,old_key_pause_state,is_pause_mode,paused);
			//printf("\n");

			//непосредственно цикл меню
			if ((is_menu_mode)&&(is_help_mode==false)){
				//if(is_pause_mode) is_pause_mode=false;						  
				// zx_machine_set_vbuf(NULL);
				zx_machine_enable_vbuf(false);
				//is_help_mode=false;
				// int num_files=get_files_from_dir("0:/z80",files[0],MAX_FILES); 
				tap_loader_active=false;
				if(!init_fs){
                    //printf("Try init FS:");
                    init_fs=init_filesystem();
					//if (init_fs) printf("Success\n"); else printf("Fail\n");
					N_files = read_select_dir(cur_dir_index);
					if(N_files==0){init_fs==false;};
				};
				if (is_new_screen){
					draw_main_window();
					
					if(init_fs){
						N_files = read_select_dir(cur_dir_index);
						//G_PRINTF_DEBUG("DIR=%s\n",dir_patch); 
						//G_PRINTF_DEBUG("N files=%d\n",N_files);
						if(N_files==0){
							init_fs=false;
						} else {
							draw_file_window();
						}
						cur_file_index_old=-1;
						draw_mur_logo_big(155,60,1);
					}					
				}//if ((is_new_screen))
				if(!init_fs){
					//draw_main_window();
					MessageBox("SD Card not found!!!","    Please REBOOT   ",CL_LT_YELLOW,CL_RED,0);
					continue;
				}
				if((is_help_mode==false)&&(init_fs)){
					if((kb_st_ps2.u[1]&KB_U1_ENTER)&&(init_fs)){ 
						if (files[cur_file_index][13]){ //выбран каталог
							if (cur_file_index==0){//на уровень выше
								if (cur_dir_index) {
									cur_dir_index--;
									N_files = read_select_dir(cur_dir_index);
									cur_file_index=0;
									draw_text_len(FONT_W,FONT_H-1,"                    ",COLOR_BACKGOUND,COLOR_BORDER,20);
									continue;
								};
							}
							if (cur_dir_index<(DIRS_DEPTH-2)){//выбор каталога
								cur_dir_index++;
								strcpy(dirs[cur_dir_index],files[cur_file_index]);
								N_files = read_select_dir(cur_dir_index);
								cur_file_index=0;
								cur_file_index_old=cur_file_index;
								//shift_file_index=0;
								last_action = time_us_32();
								continue;
							}
						} else {  // выбран файл
							strcpy(activefilename,dir_patch);
							strcat(activefilename,"/");
							strcat(activefilename,files[cur_file_index]);
							afilename[0]=0;
							strcat(afilename,files[cur_file_index]);							
							const char* ext = get_file_extension(activefilename);
							if(strcasecmp(ext, "z80") == 0) {
								//G_PRINTF_DEBUG("current file select=%s\n",activefilename); 
								//load_image_z80(activefilename);
								im_z80_stop = true;
								while (im_z80_stop){
									sleep_ms(10);
									if (im_ready_loading){
										//sleep_ms(10);
										zx_machine_reset();
										AY_reset();// сбросить AY
										//load_image_z80(activefilename);
										
										if (load_image_z80(activefilename)){
											memset(temp_msg,0,sizeof(temp_msg));
											sprintf(temp_msg," Loading file:%s",afilename);
											MessageBox("Z80",temp_msg,CL_WHITE,CL_BLUE,2);
											activefilename[0]=0;
											im_z80_stop = false;					   
											im_ready_loading = false;
											paused=0;
											is_menu_mode=false;
											//printf("load_image_z80 - OK\n");
											break;
										} else {
											MessageBox("Error loading snapshot!!!",afilename,CL_YELLOW,CL_LT_RED,1);
											//printf("load_image_z80 - ERROR\n");
											last_action = time_us_32();
											draw_file_window();
											im_z80_stop = false;
											im_ready_loading = false;
											paused=0;
											break;
										}
										//AY_reset();// сбросить AY
									}
								}							
								
								continue;
							} else
							if(strcasecmp(ext, "sna") == 0) {
								//G_PRINTF_DEBUG("current file select=%s\n",activefilename); 
								//load_image_z80(activefilename);
								im_z80_stop = true;
								while (im_z80_stop){
									sleep_ms(10);
									if (im_ready_loading){
										zx_machine_reset();
										AY_reset();// сбросить AY
										if (load_image_sna(activefilename)){
											memset(temp_msg,0,sizeof(temp_msg));
											sprintf(temp_msg," Loading file:%s",afilename);
											MessageBox("SNA",temp_msg,CL_WHITE,CL_BLUE,2);
											activefilename[0]=0;
											im_z80_stop = false;					   
											im_ready_loading = false;
											paused=0;
											is_menu_mode=false;
											//printf("load_image_sna - OK\n");
											break;
										} else {
											MessageBox("Error loading snapshot!!!",afilename,CL_YELLOW,CL_LT_RED,1);
											//printf("load_image_sna - ERROR\n");
											last_action = time_us_32();
											draw_file_window();
											im_z80_stop = false;
											im_ready_loading = false;
											paused=0;
											break;
										}
										//AY_reset();// сбросить AY
									}
								}							
								continue;
							} else 							 
							if(strcasecmp(ext, "scr") == 0) {
								//G_PRINTF_DEBUG("current file select=%s\n",activefilename); 
								if(LoadScreenshot(activefilename,true)){
									is_menu_mode=false;
									continue;
								} else {
									MessageBox("Error loading screen!!!",afilename,CL_YELLOW,CL_LT_RED,1);
									//printf("LoadScreenshot - ERROR\n");
									break;
								}
							} else 
							if(strcasecmp(ext, "tap") == 0) {
								paused=0;
								zx_machine_enable_vbuf(true);
								im_z80_stop=false;
								g_delay_ms(20);
								is_menu_mode=false;
								tap_loader_active = false;
								printf("TAP prepare\n");
								if(TAP_Load(activefilename)){
									printf("TAP loaded\n");
									memset(temp_msg,0,sizeof(temp_msg));
									sprintf(temp_msg," Loading:%s ",afilename);
									MessageBox("TAPE",temp_msg,CL_WHITE,CL_BLUE,4);
									tape_autoload_status = 2;
									tap_loader_active = true;
									last_action = time_us_32();
									memset(temp_msg,0,sizeof(temp_msg));
								} else {
									activefilename[0] = 0;
									MessageBox("ERROR","Loading tape!!!",CL_LT_YELLOW,CL_LT_RED,1);
									printf("Tap ERROR\n");
									im_z80_stop = false;
									im_ready_loading = false;
									//zx_machine_reset();
									continue;
								}
								continue;
								//sleep_ms(5);
							}

///////////////////////////////////////////////////////////////////////////////////
// tr-dos
							if(strcasecmp(ext, "trd") == 0){
								paused=0;						
								g_delay_ms(20);
								is_menu_mode=false;								
								if (OpenTRDFile(activefilename)) {

									memset(temp_msg,0,sizeof(temp_msg));
									sprintf(temp_msg," file:  %s",afilename);
									MessageBox("TRD mount",temp_msg,CL_WHITE,CL_BLUE,2);
									is_menu_mode=false;
									paused=20;



								}


							}




/* 

                             
								
							//	G_PRINTF_DEBUG("current file select=%s\n",activefilename); 
																


								if(TRD_load(activefilename)){
									memset(temp_msg,0,sizeof(temp_msg));
									sprintf(temp_msg," file:  %s",afilename);
									MessageBox("TRD mount",temp_msg,CL_WHITE,CL_BLUE,2);
									is_menu_mode=false;
									paused=0;
                            
							  WD1793_Init();
								//TRDOS_disabled = false;
							
                              //trdos_switch ();

                                continue;
								} else {
									MessageBox("Error setting TRD!!!",afilename,CL_YELLOW,CL_LT_RED,1);
									printf("Setting TRD - ERROR\n");									
									break;
								}
								
							}



 */

///////////////////////////////////////////////////////////////////////////////////
						}
					}
					
					int num_show_files=27;
					//стрелки вверх вниз
					if((kb_st_ps2.u[2]&KB_U2_DOWN)&&(cur_file_index<(N_files))){ cur_file_index++;last_action = time_us_32();}
					if((kb_st_ps2.u[2]&KB_U2_UP)&&(cur_file_index>0)){cur_file_index--;last_action = time_us_32();}
					//начало и конец списка
					if((kb_st_ps2.u[2]&KB_U2_LEFT)){cur_file_index=0; shift_file_index=0;last_action = time_us_32();}
					if((kb_st_ps2.u[2]&KB_U2_RIGHT)){cur_file_index=N_files; shift_file_index=(N_files>=num_show_files)?N_files-num_show_files:0;last_action = time_us_32();}
					
					//PAGE_UP PAGE_DOWN
					if((kb_st_ps2.u[2]&KB_U2_PAGE_DOWN)&&(cur_file_index<(N_files))){cur_file_index+=num_show_files;last_action = time_us_32();}
					if((kb_st_ps2.u[2]&KB_U2_PAGE_UP)&&(cur_file_index>0)){cur_file_index-=num_show_files;last_action = time_us_32();}
					//Возврат на уровень выше по BACKSPACE
					if((kb_st_ps2.u[1]&KB_U1_BACK_SPACE)){
						if (cur_dir_index==0){
							if (cur_file_index==0) cur_file_index=1;//не можем выбрать каталог вверх
							if (shift_file_index==0) shift_file_index=1;//не отображаем каталог вверх
							read_select_dir(cur_dir_index);
						} else {
							cur_dir_index--;
							N_files = read_select_dir(cur_dir_index);
							cur_file_index=0;
							draw_text_len(FONT_W,FONT_H-1,"                    ",COLOR_BACKGOUND,COLOR_BORDER,20);
							cur_file_index = 0;
							shift_file_index = 0;
							continue;							
						}
					}					
					if (cur_file_index<0) cur_file_index=0;
					if (cur_file_index>=N_files) cur_file_index=N_files;
					
					for (int i=num_show_files; i--;){
						if ((cur_file_index-shift_file_index)>=(num_show_files)) shift_file_index++;
						if ((cur_file_index-shift_file_index)<0) shift_file_index--;
					}
					
					//ограничения корневого каталога
					if (cur_dir_index==0){
						if (cur_file_index==0) cur_file_index=1;//не можем выбрать каталог вверх
						if (shift_file_index==0) shift_file_index=1;//не отображаем каталог вверх
					}
					
					//прорисовка
					//заголовок окна - текущий каталог		
					//draw_text_len(FONT_W,FONT_H-1,dir_patch+2,COLOR_TEXT,COLOR_FULLSCREEN,20);
					//sprintf(save_file_name_image,"0:/save/__F%d.Z80 ",inx_f1);
					
					//printf("Dir:%s",dir_patch);
					if (strlen(dir_patch+2)>0){
						draw_text_len(FONT_W+1,FONT_H-1,dir_patch+1,CL_WHITE,COLOR_BORDER,20);
						} /*else {
						draw_text_len(FONT_W,FONT_H-1,"                    ",COLOR_BACKGOUND,COLOR_BORDER,20);
					}*/
					
					for(int i=0;i<num_show_files;i++){
						uint8_t color_text=CL_GREEN;
						uint8_t color_text_d=CL_YELLOW;// если директория
						uint8_t color_bg=COLOR_BACKGOUND;
						
						if (i==(cur_file_index-shift_file_index)){
							//color_text=COLOR_SELECT_TEXT;//???????????????????
							//color_bg=COLOR_SELECT;//????????????????????????
							color_text=CL_BLACK;
							color_bg=COLOR_SELECT;
							color_text_d=CL_BLACK;
						}
						//если файлов меньше, чем отведено экрана - заполняем пустыми строками
						if ((i>N_files)||((cur_dir_index==0)&&(i>(N_files-1)))){
							draw_text_len(FONT_W,2*FONT_H+i*FONT_H," ",color_text,color_bg,14);
							continue;
						}

						if (files[i+shift_file_index][13]){
						

							draw_text_len(1*FONT_W,2*FONT_H+i*FONT_H,files[i+shift_file_index],color_text_d,color_bg,14);//get_file_from_dir("0:/z80",i)//?????
						} else {
							
						
						
							draw_text_len(1*FONT_W,2*FONT_H+i*FONT_H,files[i+shift_file_index],color_text,color_bg,14);//get_file_from_dir("0:/z80",i)//???????
						}						

					}
					//draw_rect(10+FONT_W*13,17,3,5,0xf,true);
					int file_inx=cur_file_index-1;
					if (file_inx==-1) file_inx=0;
					if (file_inx==N_files) file_inx+=1;
					int shft=208*(file_inx)/(N_files<=1?1:N_files-1); 
					draw_rect(9+FONT_W*14,16,6,SCREEN_H-25,COLOR_BACKGOUND,true);  //Заливка фона полосы прокрутки
					draw_rect(10+FONT_W*14,shft+17,4,5,COLOR_TEXT,true);  //указатель полосы прокрутки
					if(strcasecmp(files[cur_file_index], "..")==0) {
						cur_file_index_old=cur_file_index;
						//draw_mur_logo();
					}
					

					
					if ((cur_file_index>0)&&(cur_file_index_old==-1)){
						last_action = time_us_32();
						//cur_file_index_old=cur_file_index;
					}
					
					//break;
				}//if(init_fs)
				//sleep_ms(100);
				//continue;
			} 

			if (is_help_mode){	
				//if(is_pause_mode) is_pause_mode=false;
				//is_menu_mode=false;
				zx_machine_enable_vbuf(false);
				if(help_mode_draw){
					//memset(kb_st_ps2.u,0,sizeof(kb_st_ps2.u));
					draw_main_window();
					draw_help_text();
					help_mode_draw=false;
				}
				/*if(kb_st_ps2.u[0]>0||kb_st_ps2.u[1]>0||kb_st_ps2.u[2]>0||kb_st_ps2.u[3]>0){
					zx_machine_enable_vbuf(true);
					is_help_mode=false;
				}*/
				//sleep_ms(100);
				last_action = time_us_32();
			}
			/*else if(is_menu_mode){
				is_new_screen=true;
			}*/
			//printf("paused:%d\n",paused);
			if((is_pause_mode)&&(paused==0)){ paused = 1;} 
			if((!is_pause_mode)&&(paused==2)){ paused = 3;} 
			//if(is_help_mode){ is_help_mode=false;continue;}
			//MessageBox("PAUSE","",CL_LT_GREEN,CL_BLUE,true);
			if(paused==1){
				showEmpySlots=false;
				im_z80_stop = true;
				while (im_z80_stop){
					sleep_ms(10);
					if (im_ready_loading){				
						for (uint8_t i=0;i<16;i++){
							AY_select_reg(i);
							sound_reg_pause[i] = AY_get_reg();
							AY_set_reg(0);
						}
						sound_reg_pause[16]=beep_data_old;
						sound_reg_pause[17]=beep_data;
						break;
					}
				}
				paused=2;
				continue;
			} else if(paused==3) {
				for (uint8_t i=0;i<16;i++){
					AY_select_reg(i);
					AY_set_reg(sound_reg_pause[i]);
				}	
				beep_data_old=sound_reg_pause[16];
				beep_data = sound_reg_pause[17];
				im_z80_stop = false;
				paused=0;
				continue;
			}
			if ((is_emulation_mode)&&(paused==2)){
				MessageBox("PAUSE","Screen refresh stopped",CL_LT_GREEN,CL_BLUE,0);
			}
			if (is_emulation_mode) {// Emulation mode

				zx_machine_enable_vbuf(true);
				//zx_machine_set_vbuf(g_gbuf);

				convert_kb_u_to_kb_zx(&kb_st_ps2,zx_input.kb_data);


				if (kb_st_ps2.u[2]&KB_U2_SCROLL_LOCK) {
					joyMode++;
					if(joyMode>=MAX_JOY_MODE){
						joyMode=0;
					}
					MessageBox("JOYSTICK",joy_text[joyMode],CL_YELLOW,CL_LT_BLUE,2);					
				};

				if(joyMode==0){
            		if (kb_st_ps2.u[2]&KB_U2_UP)   {zx_input.kb_data[0]|=1<<0;zx_input.kb_data[4]|=1<<3;};
            		if (kb_st_ps2.u[2]&KB_U2_DOWN) {zx_input.kb_data[0]|=1<<0;zx_input.kb_data[4]|=1<<4;};
            		if (kb_st_ps2.u[2]&KB_U2_LEFT) {zx_input.kb_data[0]|=1<<0;zx_input.kb_data[3]|=1<<4;};
            		if (kb_st_ps2.u[2]&KB_U2_RIGHT){zx_input.kb_data[0]|=1<<0;zx_input.kb_data[4]|=1<<2;};
				}
				if(joyMode==1){
					//memset(kb_st_ps2.u,0,sizeof(kb_st_ps2.u));
					data_joy=0b11111111;
					if(kb_st_ps2.u[2]&KB_U2_RIGHT){
						data_joy^=0b00000001;
						//printf("KBD Right\n");
					};
					if(kb_st_ps2.u[2]&KB_U2_LEFT){
						data_joy^=0b00000010;
						//printf("KBD Left\n");
					};
					if(kb_st_ps2.u[2]&KB_U2_DOWN){
						data_joy^=0b00000100;
						//printf("KBD Down\n");
					};
					if(kb_st_ps2.u[2]&KB_U2_UP){
						data_joy^=0b00001000;
						//printf("KBD Up\n");
					};
					if(kb_st_ps2.u[1]&KB_U1_R_ALT){
						data_joy^=0b00010000;
						//printf("KBD Alt\n");
					};
				}
				if(joyMode==2){
            		if (kb_st_ps2.u[2]&KB_U2_UP)    {zx_input.kb_data[4]|=1<<1;};
            		if (kb_st_ps2.u[2]&KB_U2_DOWN)  {zx_input.kb_data[4]|=1<<2;};
            		if (kb_st_ps2.u[2]&KB_U2_LEFT)  {zx_input.kb_data[4]|=1<<4;};
            		if (kb_st_ps2.u[2]&KB_U2_RIGHT) {zx_input.kb_data[4]|=1<<3;};
					if (kb_st_ps2.u[1]&KB_U1_R_ALT) {zx_input.kb_data[4]|=1<<0;};
				}
				if(joyMode==3){
            		if (kb_st_ps2.u[2]&KB_U2_UP)    {zx_input.kb_data[4]|=1<<3;};
            		if (kb_st_ps2.u[2]&KB_U2_DOWN)  {zx_input.kb_data[4]|=1<<4;};
            		if (kb_st_ps2.u[2]&KB_U2_LEFT)  {zx_input.kb_data[3]|=1<<4;};
            		if (kb_st_ps2.u[2]&KB_U2_RIGHT) {zx_input.kb_data[4]|=1<<2;};
					if (kb_st_ps2.u[1]&KB_U1_R_ALT) {zx_input.kb_data[4]|=1<<0;};
				}					
				//printf("data_joy^%x\n",data_joy);
				/*--Emulator reset--*/
				if (((kb_st_ps2.u[1]&KB_U1_L_CTRL)||(kb_st_ps2.u[1]&KB_U1_R_CTRL))&&((kb_st_ps2.u[1]&KB_U1_L_ALT)||(kb_st_ps2.u[1]&KB_U1_R_ALT))&&(kb_st_ps2.u[2]&KB_U2_DELETE)){
					//G_PRINTF_INFO("restart\n");
					zx_machine_reset();
				}
				
				//AY_print_state_debug();
				if ((kb_st_ps2.u[1]&KB_U1_MENU)||(kb_st_ps2.u[3]&KB_U3_F12)){
					if (showEmpySlots==true){
						showEmpySlots=false;
					} else
					if (showEmpySlots==false){
						showEmpySlots=true;
					}
					if (showEmpySlots){
						memset(sound_reg_pause,0,sizeof(sound_reg_pause));
						for(uint8_t i=1;i<11;i++){
							sprintf(save_file_name_image,"0:/save/__F%d.Z80 ",i);
							fr = sd_open_file(&f,save_file_name_image,FA_READ);
							if(fr==FR_OK){sound_reg_pause[i]=1;}
						}
					} else {
						memset(sound_reg_pause,0,sizeof(sound_reg_pause));
					}
				}

				/*--Fast Save--*/
				if ((kb_st_ps2.u[1]&KB_U1_L_SHIFT || kb_st_ps2.u[1]&KB_U1_R_SHIFT)||(kb_st_ps2.u[1]&KB_U1_L_CTRL || kb_st_ps2.u[1]&KB_U1_R_CTRL)){
					uint inx_f1=0;
					//char file_list;
					switch (kb_st_ps2.u[3]){
						case KB_U3_F1:  inx_f1=1;break;
						case KB_U3_F2:  inx_f1=2;break;
						case KB_U3_F3:  inx_f1=3;break;
						case KB_U3_F4:  inx_f1=4;break;
						case KB_U3_F5:  inx_f1=5;break;
						case KB_U3_F6:  inx_f1=6;break;
						case KB_U3_F7:  inx_f1=7;break;
						case KB_U3_F8:  inx_f1=8;break;
						case KB_U3_F9:  inx_f1=9;break;
						case KB_U3_F10: inx_f1=10;break;
						
						/*case KB_U3_F11: inx_f1=11;break;
							//case KB_U3_F11: z80_gen_nmi_from_main = true;break;
							
							//case KB_U3_F12: inx_f1=12;break;
							case KB_U3_F12: 
							// //inx_f1=12;
							// int num_files=get_files_from_dir("0:/",files[0],MAX_FILES);	 
							//	 for(uint8_t i=1;i<20;i++)
							//	 {
							//		 //printf("[%s] %d\n",file_list[i],strlen(file_list[i]));
							//	 }		   
							// get_files_from_dir("0:/z80");
							
						break;*/
						
						default:
						break;
					}//switch (kb_st_ps2.u[3])

					if (inx_f1>0&&inx_f1<11){
						sprintf(save_file_name_image,"0:/save/__F%d.Z80 ",inx_f1);
						//save_file_name_image[11]=48+inx_f1;
						//G_PRINTF("save filename = %s \n",save_file_name_image);
						showEmpySlots=false;
						if (kb_st_ps2.u[1]&KB_U1_L_SHIFT || kb_st_ps2.u[1]&KB_U1_R_SHIFT){
							im_z80_stop = true;
							while (im_z80_stop){
								sleep_ms(10);
								if (im_ready_loading){
									//sleep_ms(10);
									//char slot_name[20];
									//draw_text(100,100,slot_name,0xf,0x1);
									zx_machine_reset();
									AY_reset();// сбросить AY
									memset(temp_msg,0,sizeof(temp_msg));
									sprintf(temp_msg," Loading slot#%d ",inx_f1);
									if (load_image_z80(save_file_name_image)){
										MessageBox("QUICKLOAD",temp_msg,CL_WHITE,CL_BLUE,2);
									} else {
										MessageBox("Error QUICKLOAD!!!",temp_msg,CL_YELLOW,CL_LT_RED,1);
									}
									//AY_reset();// сбросить AY
									memset(kb_st_ps2.u,0,sizeof(kb_st_ps2.u));
									is_help_mode=false;
									im_z80_stop = false;					   
									im_ready_loading = false;
									
									break;
								}
							}
							continue;
						}
						if (kb_st_ps2.u[1]&KB_U1_L_CTRL || kb_st_ps2.u[1]&KB_U1_R_CTRL){
							im_ready_loading = false;
							im_z80_stop = true;
							while (im_z80_stop){
								sleep_ms(10);
								if (im_ready_loading){
									sleep_ms(10);
									int fd = sd_mkdir("0:/save");
									if ((fd!=FR_OK)&&(fd!=FR_EXIST) ){
										memset(temp_msg,0,sizeof(temp_msg));
										sprintf(temp_msg," Error saving slot#%d ",inx_f1);
										MessageBox("QUICKSAVE",temp_msg,CL_LT_YELLOW,CL_RED,1);
										//draw_text(90,100,slot_name,0x0E,0x02);
										//printf("Error - mkdir %d\n",fd);
										break;	
									}
									//char slot_name[20];
									memset(temp_msg,0,sizeof(temp_msg));
									sprintf(temp_msg," Saving slot#%d ",inx_f1);
									//draw_text(100,100,slot_name,0xf,0x1);
									MessageBox("QUICKSAVE",temp_msg,CL_WHITE,CL_BLUE,2);
									save_image_z80(save_file_name_image);
									sleep_ms(100);
									im_z80_stop = false;					   
									im_ready_loading = false;
									sleep_ms(100);
									break;
								}
							}			   
							continue;
						}
					}
				}


    			if (kb_st_ps2.u[3]&KB_U3_F5){
   	    			// Start .tap reproduction
       				if (tapeFileSize==0) {
						MessageBox("ERROR","Tape file not loaded!",CL_YELLOW,CL_LT_RED,2);
       				} else {
						tap_block_percent = 0;
						tap_loader_active = true;
						tape_autoload_status = 2;
						printf("TAP Start\n");
            			TAP_Play();
   	    			}
   				}
				if ((kb_st_ps2.u[3]&KB_U3_F6)&&(tap_loader_active)) {
   	    			// Stop .tap reproduction
					MessageBox("TAPE","Stopped.",CL_GREEN,CL_LT_BLUE,2);
					printf("TAP Stop\n");
					TapeStatus=TAPE_STOPPED;
					tap_loader_active=false;
					continue;
				}
				if ((kb_st_ps2.u[3]&KB_U3_F6)&&(!tap_loader_active)) {
					MessageBox("TAPE","Rewind.",CL_GREEN,CL_LT_BLUE,2);
					tape_autoload_status=0;
					tap_block_percent = 0;
					TAP_Rewind();
					continue;
   				}
				if (kb_st_ps2.u[3]&KB_U3_F7) {
					MessageBox("TAPE","Previous block.",CL_GREEN,CL_LT_BLUE,2);
					tap_block_percent = round((tapeTotByteCount*100)/tapeFileSize);
					TAP_PrevBlock();
					continue;
				}
				if (kb_st_ps2.u[3]&KB_U3_F8) {
					MessageBox("TAPE","Next block.",CL_GREEN,CL_LT_BLUE,2);
					tap_block_percent = round((tapeTotByteCount*100)/tapeFileSize);
					TAP_NextBlock();
					continue;
				}
				if(tap_loader_active){
					last_action = time_us_32();
				}

			} 
			//zx_machine_input_set(&zx_input);
		};//if (decode_PS2())
		
		//g_delay_ms(1);
		//опрос джоя
		if ((inx++%50)==0){
			if(joyMode==0){
				data_joy=d_joy_get_data();		
				data_joy=(data_joy&0x0f)|((data_joy>>2)&0x30)|((data_joy<<3)&0x80)|((data_joy<<1)&0x40);
			};
		}
		zx_input.kempston=(uint8_t)(~data_joy);
		zx_machine_input_set(&zx_input);			
		// if (!(inx++&0xF))  new_data_joy=d_joy_get_data();   
		// if (!(inx++&0xF)) {/*Joy_get_data(&joy1);*///Joy_get_data(&joy2);}
		// //continue;
		
		// //if(Joy_is_new_data(&joy1))Joy_to_zx(&joy1,&zx_input);
		// if(Joy_is_new_data(&joy2))Joy_to_zx(&joy2,&zx_input);
		
		//zx_machine_input_set(&zx_input);	
		
		//  if (new_data_joy!=data_joy)
		// {
		// data_joy=new_data_joy;
		
		//zx_input.kempston=(~data_joy);
		//data_joy=new_data_joy;
		
		// G_PRINTF_INFO("data joy=0x%02x\n",~data_joy);
		//g_delay_ms(1000);
		// }

		if ((is_menu_mode)&&(is_help_mode==false)&&(init_fs)){
			if((last_action>0)&&(time_us_32()-last_action)>SHOW_SCREEN_DELAY*1000){
				//printf("Timers: LA:%d GB:%d\n",last_action,time_us_32());
				last_action=0;
				draw_rect(17+FONT_W*14,209,182,22,COLOR_BACKGOUND,true);//Фон отображения информации о файле				
				const char* ext = get_file_extension(files[cur_file_index]);
//-----------------------------------------------
//TRD INFO
if(strcasecmp(ext, "trd")==0) {


					strcpy(activefilename,dir_patch);
					strcat(activefilename,"/");
					strcat(activefilename,files[cur_file_index]);
					cur_file_index_old=cur_file_index;


				if(!ReadCatalog(activefilename,false)){

				 } 
							

continue;
}
//-----------------------------------------------------





				if(strcasecmp(ext, "z80")==0) {
					strcpy(activefilename,dir_patch);
					strcat(activefilename,"/");
					strcat(activefilename,files[cur_file_index]);
					cur_file_index_old=cur_file_index;
					//if(LoadScreenFromZ80Snapshot(activefilename)) printf("show - OK \n"); else printf("screen not found");
				//	if(!LoadScreenFromZ80Snapshot(activefilename)) draw_mur_logo();
					continue;
				} else
				if(strcasecmp(ext, "scr") == 0) {
					strcpy(activefilename,dir_patch);
					strcat(activefilename,"/");
					strcat(activefilename,files[cur_file_index]);
					//printf("LoadScreenshot: %s\n",activefilename);
					LoadScreenshot(activefilename,false);
					cur_file_index_old=cur_file_index;
					continue;
				} else
				if(strcasecmp(ext, "tap") == 0) {
					strcpy(activefilename,dir_patch);
					strcat(activefilename,"/");
					strcat(activefilename,files[cur_file_index]);
					//printf("LoadScreenshot: %s\n",activefilename);
					draw_rect(17+FONT_W*14,16,182,191,COLOR_BACKGOUND,true);
					LoadScreenFromTap(activefilename);
					cur_file_index_old=cur_file_index;
					continue;
				} 
				if(strcasecmp(ext, "sna") == 0) {
					strcpy(activefilename,dir_patch);
					strcat(activefilename,"/");
					strcat(activefilename,files[cur_file_index]);
					//printf("LoadScreenshot: %s\n",activefilename);
					draw_rect(17+FONT_W*14,16,182,191,COLOR_BACKGOUND,true);
					LoadScreenFromSNASnapshot(activefilename);
					cur_file_index_old=cur_file_index;
					continue;
				} 
				else {
					if (cur_file_index_old==-1){
						draw_rect(17+FONT_W*14,16,182,192,COLOR_BACKGOUND,true);//Фон отображения скринов						
						draw_mur_logo_big(155,60,1);
						} else {
						draw_rect(17+FONT_W*14,16,182,191,COLOR_PIC_BG,true);	
					//	draw_mur_logo();
					};
					cur_file_index_old=cur_file_index;
					//printf("Draw Mur Logo \n");
				}
			}
		}


		if ((tap_loader_active)&&(!(is_menu_mode|is_help_mode))){
			if((last_action>0)&&(time_us_32()-last_action)>SHOW_SCREEN_DELAY*100){
				tap_block_percent = round((tapeTotByteCount*100)/tapeFileSize);//относительно 100%
				last_action = time_us_32();
				//printf("Loaded [%%%d]\n",tap_block_percent);
				memset(temp_msg,0,sizeof(temp_msg));
				/*if ((tapeCurrentBlock>0)&&(tap_blocks[tapeCurrentBlock-1].Flag==0)){
					sprintf(temp_msg,"[%%%d]:%s %s %d",tap_block_percent,files[cur_file_index],tap_blocks[tapeCurrentBlock-1].NAME,tap_blocks[tapeCurrentBlock-1].Size);
				} else {
					sprintf(temp_msg,"[%%%d]:%s %d",tap_block_percent,files[cur_file_index],tap_blocks[tapeCurrentBlock-1].Size);
				}*/
				sprintf(temp_msg,"[%%%d]:%s BK:%d %dKb",tap_block_percent,afilename,tapeCurrentBlock,(tap_blocks[tapeCurrentBlock].Size/1024));
				
				
			}
			draw_text(32,24+192,temp_msg,CL_LT_YELLOW,CL_LT_BLUE);//24+192 //208
			//g_delay_ms(250);
		}
		if(showEmpySlots){
			for(uint8_t i=1;i<11;i++){
				memset(save_file_name_image,0,sizeof(save_file_name_image));
				sprintf(save_file_name_image,"[%d]",i);
				draw_text(8+(i*24),32+192,save_file_name_image,sound_reg_pause[i]==1?CL_RED:CL_GREEN,CL_GRAY);//24+192 //208
			}
		}

	
		
	}//while(1)
	software_reset();
}
