#include "../input/keycodes.hpp"

unsigned char keycodeToChar(uint32_t keycode){
    if(keycode >= 7 && keycode <= 16)return (char)(keycode - 7 + 0x30);
    if(keycode >= 29 && keycode <= 54)return (char)(keycode - 29 + 0x41);
    if(keycode >  131 && keycode <= 131+12)return SPECIAL_KEY_F(keycode-131);
    switch(keycode){
        case 61: return '\t';
        case 66: return '\n';
        case 62: return ' ';
        case 59:
        case 60: return SPECIAL_KEY_SHIFT;
        case 57:
        case 58: return SPECIAL_KEY_ALT;
        case 113:
        case 114: return SPECIAL_KEY_CTRL;
        case 111: return SPECIAL_KEY_ESC;
        default: return 0;
    }
}

uint32_t charToKeycode(unsigned char c){
    if(c >= 0x30 && c <= 0x39) return c - 0x30 + 7;
    if(c >= 0x41 && c <= 0x5a) return c - 0x41 + 29;
    if(c >= 0x61 && c <= 0x7a) return c - 0x61 + 29;
    if(c > SPECIAL_KEY_F_BASE){
        c -= SPECIAL_KEY_F_BASE;
        if(c <= 12)return 131 + c;
        else return 0;
    }
    switch(c){
        case '\t': return 61;
        case '\n': return 66;
        case ' ': return 62;
        case SPECIAL_KEY_SHIFT: return 59;
        case SPECIAL_KEY_ALT: return 57;
        case SPECIAL_KEY_CTRL: return 113;
        case SPECIAL_KEY_ESC: return 114;
        default: return 0;
    }
}
