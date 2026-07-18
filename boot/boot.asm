; NovaOS Stage-1 bootloader (MBR)
; Loads stage2+kernel from disk and jumps to stage2 at 0x7E00

BITS 16
ORG 0x7C00

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [boot_drive], dl

    ; Clear screen
    mov ax, 0x0003
    int 0x10

    mov si, msg_loading
    call print

    ; Load sectors starting at LBA 1 into 0x7E00
    ; Read STAGE2_SECTORS + KERNEL_SECTORS
    mov ah, 0x02
    mov al, 80          ; sectors to read (40 KB)
    mov ch, 0           ; cylinder
    mov cl, 2           ; start sector (1-based; sector 2)
    mov dh, 0           ; head
    mov dl, [boot_drive]
    mov bx, 0x7E00
    int 0x13
    jc disk_error

    jmp 0x0000:0x7E00

disk_error:
    mov si, msg_disk
    call print
.hang:
    hlt
    jmp .hang

print:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    mov bh, 0
    int 0x10
    jmp print
.done:
    ret

boot_drive: db 0
msg_loading: db "NovaOS booting...", 13, 10, 0
msg_disk:    db "Disk read error", 13, 10, 0

times 510-($-$$) db 0
dw 0xAA55
