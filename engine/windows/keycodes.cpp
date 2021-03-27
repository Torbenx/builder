#include "../input/keycodes.hpp"
#include <Windows.h>
#include "../util/log.hpp"

unsigned char keycodeToChar(uint32_t keycode){
    if(keycode >= 0x30 && keycode <= 0x39)return (char)keycode;
    if(keycode >= 0x41 && keycode <= 0x5a)return (char)keycode;
    if(keycode >  0x6f && keycode <= 0x6f+24)return SPECIAL_KEY_F(keycode-0x6f);
    switch(keycode){
        case 0x09: return '\t';
        case 0x0a: return '\n';
        case 0x20: return ' ';
        case 0x10: return SPECIAL_KEY_SHIFT;
        case 0x12: return SPECIAL_KEY_ALT;
        case 0x11: return SPECIAL_KEY_CTRL;
        case 0x1b: return SPECIAL_KEY_ESC;
        case 0x26: return SPECIAL_KEY_UP;
        case 0x28: return SPECIAL_KEY_DOWN;
        case 0x25: return SPECIAL_KEY_LEFT;
        case 0x27: return SPECIAL_KEY_RIGHT;
        default: return 0;
    }
}

uint32_t charToKeycode(unsigned char c){
    if(c >= '0' && c <= '9') return c;
    if(c >= 'A' && c <= 'Z') return c;
    if(c >= 'a' && c <= 'z') return c-'a'+'A';
    if(c > SPECIAL_KEY_F_BASE){
        c -= SPECIAL_KEY_F_BASE;
        if(c <= 24)return 0x6f + c;
        else return 0;
    }
    switch(c){
        case '\t': return 0x09;
        case '\n': return 0x0a;
        case ' ': return 0x20;
        case SPECIAL_KEY_SHIFT: return 0x10;
        case SPECIAL_KEY_ALT: return 0x12;
        case SPECIAL_KEY_CTRL: return 0x11;
        case SPECIAL_KEY_ESC: return 0x1b;
        case SPECIAL_KEY_UP: return 0x26;
        case SPECIAL_KEY_DOWN: return 0x28;
        case SPECIAL_KEY_LEFT: return 0x25;
        case SPECIAL_KEY_RIGHT: return 0x27;
        default: return 0;
    }
}
