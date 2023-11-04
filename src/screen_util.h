#pragma once
#include "inttypes.h"
#include "stdbool.h"

//#define FONT6X8   
#define FONT8X8  

#ifdef FONT8X8 
#include "font8x8.h"
#define FONT   FONT_8x8_DATA
#endif

#ifdef FONT6X8 
#include "font6x8revers.h"
#define FONT   FONT_6x8_DATA
#endif

/*
color:
    0x00 - black
    0x01 - blue
    0x02 - red
    0x03 - pink
    0x04 - green
    0x05 - cyan
    0x06 - yellow
    0x07 - gray
    0x08 - black
    0x09 - blue+
    0x0a - red+
    0x0b - pink+
    0x0c - green+
    0x0d - cyan+
    0x0e - yellow+
    0x0f - white
*/


#define CL_BLACK     0x00
#define CL_BLUE      0x01
#define CL_RED       0x02
#define CL_PINK      0x03
#define CL_GREEN     0x04
#define CL_CYAN      0x05
#define CL_YELLOW    0x06
#define CL_GRAY      0x07
#define CL_LT_BLACK  0x08
#define CL_LT_BLUE   0x09
#define CL_LT_RED    0x0a
#define CL_LT_PINK   0x0b
#define CL_LT_GREEN  0x0c
#define CL_LT_CYAN   0x0d
#define CL_LT_YELLOW 0x0e
#define CL_WHITE     0x0f
/*
#define COLOR_TEXT          0x00
#define COLOR_SELECT        0x0D
#define COLOR_SELECT_TEXT   0x00
#define COLOR_BACKGOUND     0x0F
#define COLOR_BORDER        0x00
#define COLOR_PIC_BG        0x07
#define COLOR_FULLSCREEN    0x07
*/
#define COLOR_TEXT         CL_GREEN
#define COLOR_SELECT        0x0D
#define COLOR_SELECT_TEXT   0x00
#define COLOR_BACKGOUND     CL_BLACK
#define COLOR_BORDER        CL_BLUE 
#define COLOR_PIC_BG        CL_BLACK
#define COLOR_FULLSCREEN    CL_BLACK




typedef uint8_t color_t;

bool draw_pixel(int x,int y,color_t color);
void draw_text(int x,int y,char* text,color_t colorText,color_t colorBg);
void draw_text_len(int x,int y,char* text,color_t colorText,color_t colorBg,int len);
void draw_line(int x0,int y0, int x1, int y1,color_t color);
//void draw_circle(int x0,int y0,  int r,color_t color);
void init_screen(uint8_t* scr_buf,int scr_width,int scr_height);
void draw_rect(int x,int y,int w,int h,color_t color,bool filled);
void ShowScreenshot(uint8_t* buffer);
