#include "kb_u_codes.h"
#include "string.h"
void keys_to_str(char* str_buf,char s_char,kb_u_state kb_state)
{
    char s_str[2];
    s_str[0]=s_char;
    s_str[1]='\0';

    str_buf[0]=0;
    strcat(str_buf,"KEY PRESSED: ");
//0 набор
    if (kb_state.u[0]&KB_U0_A) {strcat(str_buf,"A");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_B) {strcat(str_buf,"B");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_C) {strcat(str_buf,"C");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_D) {strcat(str_buf,"D");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_E) {strcat(str_buf,"E");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_F) {strcat(str_buf,"F");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_G) {strcat(str_buf,"G");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_H) {strcat(str_buf,"H");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_I) {strcat(str_buf,"I");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_J) {strcat(str_buf,"J");strcat(str_buf,s_str);};

    if (kb_state.u[0]&KB_U0_K) {strcat(str_buf,"K");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_L) {strcat(str_buf,"L");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_M) {strcat(str_buf,"M");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_N) {strcat(str_buf,"N");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_O) {strcat(str_buf,"O");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_P) {strcat(str_buf,"P");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_Q) {strcat(str_buf,"Q");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_R) {strcat(str_buf,"R");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_S) {strcat(str_buf,"S");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_T) {strcat(str_buf,"T");strcat(str_buf,s_str);};

    if (kb_state.u[0]&KB_U0_U) {strcat(str_buf,"U");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_V) {strcat(str_buf,"V");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_W) {strcat(str_buf,"W");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_X) {strcat(str_buf,"X");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_Y) {strcat(str_buf,"Y");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_Z) {strcat(str_buf,"Z");strcat(str_buf,s_str);};

    if (kb_state.u[0]&KB_U0_SEMICOLON) {strcat(str_buf,";");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_QUOTE) {strcat(str_buf,"\"");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_COMMA) {strcat(str_buf,",");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_PERIOD) {strcat(str_buf,".");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_LEFT_BR) {strcat(str_buf,"[");strcat(str_buf,s_str);};
    if (kb_state.u[0]&KB_U0_RIGHT_BR) {strcat(str_buf,"]");strcat(str_buf,s_str);};
//1 набор
    if (kb_state.u[1]&KB_U1_0) {strcat(str_buf,"0");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_1) {strcat(str_buf,"1");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_2) {strcat(str_buf,"2");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_3) {strcat(str_buf,"3");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_4) {strcat(str_buf,"4");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_5) {strcat(str_buf,"5");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_6) {strcat(str_buf,"6");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_7) {strcat(str_buf,"7");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_8) {strcat(str_buf,"8");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_9) {strcat(str_buf,"9");strcat(str_buf,s_str);};

    if (kb_state.u[1]&KB_U1_ENTER) {strcat(str_buf,"ENTER");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_SLASH) {strcat(str_buf,"/");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_MINUS) {strcat(str_buf,"MINUS");strcat(str_buf,s_str);};

    if (kb_state.u[1]&KB_U1_EQUALS) {strcat(str_buf,"=");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_BACKSLASH) {strcat(str_buf,"BACKSLASH");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_CAPS_LOCK) {strcat(str_buf,"CAPS_LOCK");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_TAB) {strcat(str_buf,"TAB");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_BACK_SPACE) {strcat(str_buf,"BACK_SPACE");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_ESC) {strcat(str_buf,"ESC");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_TILDE) {strcat(str_buf,"TILDE");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_MENU) {strcat(str_buf,"MENU");strcat(str_buf,s_str);};

    if (kb_state.u[1]&KB_U1_L_SHIFT) {strcat(str_buf,"L_SHIFT");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_L_CTRL) {strcat(str_buf,"L_CTRL");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_L_ALT) {strcat(str_buf,"L_ALT");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_L_WIN) {strcat(str_buf,"L_WIN");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_R_SHIFT) {strcat(str_buf,"R_SHIFT");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_R_CTRL) {strcat(str_buf,"R_CTRL");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_R_ALT) {strcat(str_buf,"R_ALT");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_R_WIN) {strcat(str_buf,"R_WIN");strcat(str_buf,s_str);};
    if (kb_state.u[1]&KB_U1_SPACE) {strcat(str_buf,"SPACE");strcat(str_buf,s_str);};


//2 набор
    if (kb_state.u[2]&KB_U2_NUM_0) {strcat(str_buf,"NUM_0");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_NUM_1) {strcat(str_buf,"NUM_1");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_NUM_2) {strcat(str_buf,"NUM_2");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_NUM_3) {strcat(str_buf,"NUM_3");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_NUM_4) {strcat(str_buf,"NUM_4");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_NUM_5) {strcat(str_buf,"NUM_5");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_NUM_6) {strcat(str_buf,"NUM_6");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_NUM_7) {strcat(str_buf,"NUM_7");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_NUM_8) {strcat(str_buf,"NUM_8");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_NUM_9) {strcat(str_buf,"NUM_9");strcat(str_buf,s_str);};

    if (kb_state.u[2]&KB_U2_NUM_ENTER) {strcat(str_buf,"ENTER");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_NUM_SLASH) {strcat(str_buf,"/");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_NUM_MINUS) {strcat(str_buf,"MINUS");strcat(str_buf,s_str);};

    if (kb_state.u[2]&KB_U2_NUM_PLUS) {strcat(str_buf,"NUM_PLUS");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_NUM_MULT) {strcat(str_buf,"NUM_MULT");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_NUM_PERIOD) {strcat(str_buf,"NUM_PERIOD");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_NUM_LOCK) {strcat(str_buf,"NUM_LOCK");strcat(str_buf,s_str);};

    if (kb_state.u[2]&KB_U2_DELETE) {strcat(str_buf,"DEL");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_SCROLL_LOCK) {strcat(str_buf,"SCROLL_LOCK");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_PAUSE_BREAK) {strcat(str_buf,"PAUSE_BREAK");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_INSERT) {strcat(str_buf,"INSERT");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_HOME) {strcat(str_buf,"HOME");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_PAGE_UP) {strcat(str_buf,"PG_UP");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_PAGE_DOWN) {strcat(str_buf,"PG_DOWN");strcat(str_buf,s_str);};

    if (kb_state.u[2]&KB_U2_PRT_SCR) {strcat(str_buf,"PRT_SCR");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_END) {strcat(str_buf,"END");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_UP) {strcat(str_buf,"UP");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_DOWN) {strcat(str_buf,"DOWN");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_LEFT) {strcat(str_buf,"LEFT");strcat(str_buf,s_str);};
    if (kb_state.u[2]&KB_U2_RIGHT) {strcat(str_buf,"RIGHT");strcat(str_buf,s_str);};

//3 набор
    if (kb_state.u[3]&KB_U3_F1) {strcat(str_buf,"F1");strcat(str_buf,s_str);};
    if (kb_state.u[3]&KB_U3_F2) {strcat(str_buf,"F2");strcat(str_buf,s_str);};
    if (kb_state.u[3]&KB_U3_F3) {strcat(str_buf,"F3");strcat(str_buf,s_str);};
    if (kb_state.u[3]&KB_U3_F4) {strcat(str_buf,"F4");strcat(str_buf,s_str);};
    if (kb_state.u[3]&KB_U3_F5) {strcat(str_buf,"F5");strcat(str_buf,s_str);};
    if (kb_state.u[3]&KB_U3_F6) {strcat(str_buf,"F6");strcat(str_buf,s_str);};
    if (kb_state.u[3]&KB_U3_F7) {strcat(str_buf,"F7");strcat(str_buf,s_str);};
    if (kb_state.u[3]&KB_U3_F8) {strcat(str_buf,"F8");strcat(str_buf,s_str);};
    if (kb_state.u[3]&KB_U3_F9) {strcat(str_buf,"F9");strcat(str_buf,s_str);};
    if (kb_state.u[3]&KB_U3_F10) {strcat(str_buf,"F10");strcat(str_buf,s_str);};
    if (kb_state.u[3]&KB_U3_F11) {strcat(str_buf,"F11");strcat(str_buf,s_str);};
    if (kb_state.u[3]&KB_U3_F12) {strcat(str_buf,"F12");strcat(str_buf,s_str);};

    strcat(str_buf,"\n");

};


void convert_kb_u_to_kb_zx(kb_u_state* kb_st,uint8_t* zx_kb)
{
    memset(zx_kb,0,8);

    if (kb_st->u[0])
    {
            if (kb_st->u[0]&KB_U0_A) {zx_kb[1]|=1<<0;};
            if (kb_st->u[0]&KB_U0_B) {zx_kb[7]|=1<<4;};
            if (kb_st->u[0]&KB_U0_C) {zx_kb[0]|=1<<3;};
            if (kb_st->u[0]&KB_U0_D) {zx_kb[1]|=1<<2;};
            if (kb_st->u[0]&KB_U0_E) {zx_kb[2]|=1<<2;};
            if (kb_st->u[0]&KB_U0_F) {zx_kb[1]|=1<<3;};
            if (kb_st->u[0]&KB_U0_G) {zx_kb[1]|=1<<4;};
            if (kb_st->u[0]&KB_U0_H) {zx_kb[6]|=1<<4;};
            if (kb_st->u[0]&KB_U0_I) {zx_kb[5]|=1<<2;};
            if (kb_st->u[0]&KB_U0_J) {zx_kb[6]|=1<<3;};

            if (kb_st->u[0]&KB_U0_K) {zx_kb[6]|=1<<2;};
            if (kb_st->u[0]&KB_U0_L) {zx_kb[6]|=1<<1;};
            if (kb_st->u[0]&KB_U0_M) {zx_kb[7]|=1<<2;};
            if (kb_st->u[0]&KB_U0_N) {zx_kb[7]|=1<<3;};
            if (kb_st->u[0]&KB_U0_O) {zx_kb[5]|=1<<1;};
            if (kb_st->u[0]&KB_U0_P) {zx_kb[5]|=1<<0;};
            if (kb_st->u[0]&KB_U0_Q) {zx_kb[2]|=1<<0;};
            if (kb_st->u[0]&KB_U0_R) {zx_kb[2]|=1<<3;};
            if (kb_st->u[0]&KB_U0_S) {zx_kb[1]|=1<<1;};
            if (kb_st->u[0]&KB_U0_T) {zx_kb[2]|=1<<4;};

            if (kb_st->u[0]&KB_U0_U) {zx_kb[5]|=1<<3;};
            if (kb_st->u[0]&KB_U0_V) {zx_kb[0]|=1<<4;};
            if (kb_st->u[0]&KB_U0_W) {zx_kb[2]|=1<<1;};
            if (kb_st->u[0]&KB_U0_X) {zx_kb[0]|=1<<2;};
            if (kb_st->u[0]&KB_U0_Y) {zx_kb[5]|=1<<4;};
            if (kb_st->u[0]&KB_U0_Z) {zx_kb[0]|=1<<1;};

            if (kb_st->u[0]&KB_U0_SEMICOLON) {zx_kb[7]|=1<<1;zx_kb[5]|=1<<1;};
            if (kb_st->u[0]&KB_U0_QUOTE) {zx_kb[7]|=1<<1;zx_kb[5]|=1<<0;};
            if (kb_st->u[0]&KB_U0_COMMA) {zx_kb[7]|=1<<1;zx_kb[7]|=1<<3;};
            if (kb_st->u[0]&KB_U0_PERIOD) {zx_kb[7]|=1<<1;zx_kb[7]|=1<<2;};
            if (kb_st->u[0]&KB_U0_LEFT_BR) {zx_kb[7]|=1<<1;zx_kb[4]|=1<<2;};
            if (kb_st->u[0]&KB_U0_RIGHT_BR) {zx_kb[7]|=1<<1;zx_kb[4]|=1<<1;};
    }

    if (kb_st->u[1])
    {
            if (kb_st->u[1]&KB_U1_0) {zx_kb[4]|=1<<0;};
            if (kb_st->u[1]&KB_U1_1) {zx_kb[3]|=1<<0;};
            if (kb_st->u[1]&KB_U1_2) {zx_kb[3]|=1<<1;};
            if (kb_st->u[1]&KB_U1_3) {zx_kb[3]|=1<<2;};
            if (kb_st->u[1]&KB_U1_4) {zx_kb[3]|=1<<3;};
            if (kb_st->u[1]&KB_U1_5) {zx_kb[3]|=1<<4;};
            if (kb_st->u[1]&KB_U1_6) {zx_kb[4]|=1<<4;};
            if (kb_st->u[1]&KB_U1_7) {zx_kb[4]|=1<<3;};
            if (kb_st->u[1]&KB_U1_8) {zx_kb[4]|=1<<2;};
            if (kb_st->u[1]&KB_U1_9) {zx_kb[4]|=1<<1;};

            if (kb_st->u[1]&KB_U1_ENTER) {zx_kb[6]|=1<<0;};
            if (kb_st->u[1]&KB_U1_SLASH) {zx_kb[7]|=1<<1;zx_kb[0]|=1<<3;};
            if (kb_st->u[1]&KB_U1_MINUS) {zx_kb[7]|=1<<1;zx_kb[6]|=1<<3;};

            if (kb_st->u[1]&KB_U1_EQUALS) {zx_kb[7]|=1<<1;zx_kb[6]|=1<<1;};
            if (kb_st->u[1]&KB_U1_BACKSLASH) {zx_kb[7]|=1<<1;zx_kb[0]|=1<<1;};
            if (kb_st->u[1]&KB_U1_CAPS_LOCK) {zx_kb[0]|=1<<0;zx_kb[3]|=1<<1;};
            if (kb_st->u[1]&KB_U1_TAB) {zx_kb[0]|=1<<0;zx_kb[3]|=1<<0;};
            if (kb_st->u[1]&KB_U1_BACK_SPACE) {zx_kb[0]|=1<<0;zx_kb[4]|=1<<0;};
            if (kb_st->u[1]&KB_U1_ESC) {zx_kb[0]|=1<<0;zx_kb[7]|=1<<0;};
            if (kb_st->u[1]&KB_U1_TILDE) {zx_kb[7]|=1<<1;zx_kb[1]|=1<<0;};
            if (kb_st->u[1]&KB_U1_MENU) {};

            if (kb_st->u[1]&KB_U1_L_SHIFT) {zx_kb[0]|=1<<0;};
            if (kb_st->u[1]&KB_U1_L_CTRL) {zx_kb[7]|=1<<1;};
            if (kb_st->u[1]&KB_U1_L_ALT) {};//ext.mode?
            if (kb_st->u[1]&KB_U1_L_WIN) {};
            if (kb_st->u[1]&KB_U1_R_SHIFT) {zx_kb[0]|=1<<0;};
            if (kb_st->u[1]&KB_U1_R_CTRL) {zx_kb[7]|=1<<1;};
                if (kb_st->u[1]&KB_U1_R_ALT) {};//ext.mode?
                if (kb_st->u[1]&KB_U1_R_WIN) {};
            if (kb_st->u[1]&KB_U1_SPACE) {zx_kb[7]|=1<<0;};

    }

    if (kb_st->u[2])
    {
            if (kb_st->u[2]&KB_U2_NUM_0) {zx_kb[4]|=1<<0;};
            if (kb_st->u[2]&KB_U2_NUM_1) {zx_kb[3]|=1<<0;};
            if (kb_st->u[2]&KB_U2_NUM_2) {zx_kb[3]|=1<<1;};
            if (kb_st->u[2]&KB_U2_NUM_3) {zx_kb[3]|=1<<2;};
            if (kb_st->u[2]&KB_U2_NUM_4) {zx_kb[3]|=1<<3;};
            if (kb_st->u[2]&KB_U2_NUM_5) {zx_kb[3]|=1<<4;};
            if (kb_st->u[2]&KB_U2_NUM_6) {zx_kb[4]|=1<<4;};
            if (kb_st->u[2]&KB_U2_NUM_7) {zx_kb[4]|=1<<3;};
            if (kb_st->u[2]&KB_U2_NUM_8) {zx_kb[4]|=1<<2;};
            if (kb_st->u[2]&KB_U2_NUM_9) {zx_kb[4]|=1<<1;};

            if (kb_st->u[2]&KB_U2_NUM_ENTER) {zx_kb[6]|=1<<0;};
            if (kb_st->u[2]&KB_U2_NUM_SLASH) {zx_kb[7]|=1<<1;zx_kb[0]|=1<<4;};
            if (kb_st->u[2]&KB_U2_NUM_MINUS) {zx_kb[7]|=1<<1;zx_kb[6]|=1<<3;};

            if (kb_st->u[2]&KB_U2_NUM_PLUS) {zx_kb[7]|=1<<1;zx_kb[6]|=1<<2;};
            if (kb_st->u[2]&KB_U2_NUM_MULT) {zx_kb[7]|=1<<1;zx_kb[7]|=1<<4;};
            if (kb_st->u[2]&KB_U2_NUM_PERIOD) {zx_kb[7]|=1<<1;zx_kb[7]|=1<<2;};

            if (kb_st->u[2]&KB_U2_NUM_LOCK) {};

            if (kb_st->u[2]&KB_U2_DELETE) {};
            if (kb_st->u[2]&KB_U2_SCROLL_LOCK) {};
            if (kb_st->u[2]&KB_U2_PAUSE_BREAK) {};
            if (kb_st->u[2]&KB_U2_INSERT) {};
            if (kb_st->u[2]&KB_U2_HOME) {};
            if (kb_st->u[2]&KB_U2_PAGE_UP) {};
            if (kb_st->u[2]&KB_U2_PAGE_DOWN) {};

            if (kb_st->u[2]&KB_U2_PRT_SCR) {};
            if (kb_st->u[2]&KB_U2_END) {};
    }


};
