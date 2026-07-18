; NovaOS Stage-2 bootloader
; Sets VESA graphics, enables A20, enters protected mode, jumps to kernel

BITS 16
ORG 0x7E00

STAGE2_START:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; --- Enable A20 ---
    call enable_a20

    ; --- Set VESA mode 0x118: 1024x768x32 ---
    mov ax, 0x4F02
    mov bx, 0x4118          ; LFB + mode 0x118
    int 0x10
    cmp ax, 0x004F
    jne .try_alt

    mov word [vbe_mode], 0x118
    jmp .get_mode_info

.try_alt:
    ; Fallback: 640x480x32 (0x112)
    mov ax, 0x4F02
    mov bx, 0x4112
    int 0x10
    cmp ax, 0x004F
    jne vesa_fail
    mov word [vbe_mode], 0x112

.get_mode_info:
    ; Query mode info into buffer at 0x5000
    mov ax, 0x4F01
    mov cx, [vbe_mode]
    mov di, 0x5000
    int 0x10
    cmp ax, 0x004F
    jne vesa_fail

    ; Copy useful fields into boot_info at 0x6000
    mov si, 0x5000
    mov di, BOOT_INFO
    ; width @ +18
    mov ax, [si + 18]
    mov [di + 0], ax
    ; height @ +20
    mov ax, [si + 20]
    mov [di + 2], ax
    ; bpp @ +25
    movzx ax, byte [si + 25]
    mov [di + 4], ax
    ; pitch @ +16
    mov ax, [si + 16]
    mov [di + 6], ax
    ; framebuffer phys @ +40
    mov eax, [si + 40]
    mov [di + 8], eax

    ; --- Load GDT and enter protected mode ---
    lgdt [gdt_desc]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:pm_entry

vesa_fail:
    mov si, msg_vesa
    call print16
.hang:
    hlt
    jmp .hang

print16:
    lodsb
    or al, al
    jz .d
    mov ah, 0x0E
    int 0x10
    jmp print16
.d: ret

enable_a20:
    ; Fast A20
    in al, 0x92
    or al, 2
    out 0x92, al
    ret

BITS 32
pm_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    ; Copy kernel from 0x8200 to 0x100000
    ; Stage2 is at 0x7E00, sized STAGE2_SIZE (padded to 4KB = 8 sectors)
    ; Kernel binary follows immediately after stage2 in the image.
    ; Disk layout: sector0=boot, sector1-8=stage2 (4KB), sector9+=kernel
    ; Loaded contiguously at 0x7E00, so kernel starts at 0x7E00+0x1000=0x8E00
    mov esi, 0x8E00
    mov edi, 0x100000
    mov ecx, 0x10000 / 4    ; copy 64KB
    rep movsd

    ; Pass boot_info pointer in ebx
    mov ebx, BOOT_INFO
    jmp 0x08:0x100000

; Boot info structure placed at fixed address
BOOT_INFO equ 0x6000
; layout: u16 width, u16 height, u16 bpp, u16 pitch, u32 framebuffer

vbe_mode: dw 0
msg_vesa: db "VESA init failed", 13, 10, 0

; --- GDT ---
align 8
gdt_start:
    dq 0
    ; code: base=0 limit=4GB
    dw 0xFFFF, 0x0000
    db 0x00, 0x9A, 0xCF, 0x00
    ; data: base=0 limit=4GB
    dw 0xFFFF, 0x0000
    db 0x00, 0x92, 0xCF, 0x00
gdt_end:

gdt_desc:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; Pad stage2 to exactly 4KB (8 sectors)
times 4096-($-$$) db 0
