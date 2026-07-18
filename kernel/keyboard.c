#include "keyboard.h"
#include "ports.h"
#include "idt.h"

static volatile char queue[64];
static volatile int qhead = 0, qtail = 0;
static bool shift = false;
static bool caps = false;

static const char keymap[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
    'a','s','d','f','g','h','j','k','l',';','\'','`', 0, '\\',
    'z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' ', 0,
};

static const char keymap_shift[128] = {
    0, 27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n', 0,
    'A','S','D','F','G','H','J','K','L',':','"','~', 0, '|',
    'Z','X','C','V','B','N','M','<','>','?', 0, '*', 0, ' ', 0,
};

static void push_char(char c) {
    int next = (qhead + 1) & 63;
    if (next == qtail) return;
    queue[qhead] = c;
    qhead = next;
}

void keyboard_handler(void) {
    u8 sc = inb(0x60);
    if (sc == 0x2A || sc == 0x36) { shift = true; return; }
    if (sc == 0xAA || sc == 0xB6) { shift = false; return; }
    if (sc == 0x3A) { caps = !caps; return; }
    if (sc & 0x80) return; /* break */

    char c = 0;
    if (sc < 128) {
        bool upper = shift ^ caps;
        c = upper ? keymap_shift[sc] : keymap[sc];
        /* letters only for caps on non-shift path already handled */
        if (!shift && caps && c >= 'a' && c <= 'z') c = (char)(c - 32);
        if (shift && caps && c >= 'A' && c <= 'Z') c = (char)(c + 32);
    }
    if (c) push_char(c);
}

void keyboard_init(void) {
    qhead = qtail = 0;
    irq_install_handler(1, keyboard_handler);
}

bool keyboard_has_char(void) { return qhead != qtail; }

char keyboard_get_char(void) {
    if (qhead == qtail) return 0;
    char c = queue[qtail];
    qtail = (qtail + 1) & 63;
    return c;
}

bool keyboard_shift(void) { return shift; }
