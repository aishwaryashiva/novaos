#ifndef NOVA_IDT_H
#define NOVA_IDT_H

#include "types.h"

void idt_init(void);
void irq_install_handler(int irq, void (*handler)(void));
void irq_uninstall_handler(int irq);

#endif
