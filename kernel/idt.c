#include "idt.h"
#include "ports.h"
#include "types.h"

struct idt_entry {
    u16 base_lo;
    u16 sel;
    u8  always0;
    u8  flags;
    u16 base_hi;
} __attribute__((packed));

struct idt_ptr {
    u16 limit;
    u32 base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idt_ptr idtp;

extern void* isr_stub_table[];
extern void* irq_stub_table[];

static void (*irq_handlers[16])(void);

static void idt_set_gate(u8 num, u32 base, u16 sel, u8 flags) {
    idt[num].base_lo = base & 0xFFFF;
    idt[num].base_hi = (base >> 16) & 0xFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

static void pic_remap(void) {
    outb(0x20, 0x11); io_wait();
    outb(0xA0, 0x11); io_wait();
    outb(0x21, 0x20); io_wait();
    outb(0xA1, 0x28); io_wait();
    outb(0x21, 0x04); io_wait();
    outb(0xA1, 0x02); io_wait();
    outb(0x21, 0x01); io_wait();
    outb(0xA1, 0x01); io_wait();
    outb(0x21, 0x00);
    outb(0xA1, 0x00);
}

void idt_init(void) {
    idtp.limit = sizeof(idt) - 1;
    idtp.base = (u32)&idt;

    for (int i = 0; i < 256; i++)
        idt_set_gate(i, 0, 0x08, 0x8E);

    for (int i = 0; i < 32; i++)
        idt_set_gate(i, (u32)isr_stub_table[i], 0x08, 0x8E);

    pic_remap();

    for (int i = 0; i < 16; i++) {
        idt_set_gate(32 + i, (u32)irq_stub_table[i], 0x08, 0x8E);
        irq_handlers[i] = 0;
    }

    __asm__ volatile ("lidt %0" : : "m"(idtp));
}

void irq_install_handler(int irq, void (*handler)(void)) {
    if (irq >= 0 && irq < 16) irq_handlers[irq] = handler;
}

void irq_uninstall_handler(int irq) {
    if (irq >= 0 && irq < 16) irq_handlers[irq] = 0;
}

struct regs {
    u32 gs, fs, es, ds;
    u32 edi, esi, ebp, esp, ebx, edx, ecx, eax;
    u32 int_no, err_code;
    u32 eip, cs, eflags, useresp, ss;
};

void isr_dispatch(struct regs* r) {
    (void)r;
}

void irq_dispatch(struct regs* r) {
    int irq = (int)r->int_no;
    if (irq >= 0 && irq < 16 && irq_handlers[irq])
        irq_handlers[irq]();

    if (irq >= 8)
        outb(0xA0, 0x20);
    outb(0x20, 0x20);
}
