bits 16

global _putInMemory
global _interrupt10, _interrupt13, _interrupt16, _interrupt21

; A simple function to write to a specific memory location
_putInMemory:
    push bp
    mov bp, sp
    push ds
    mov ax, [bp+4]  ; segment
    mov si, [bp+6]  ; address
    mov cl, [bp+8]  ; character
    mov ds, ax
    mov [si], cl
    pop ds
    pop bp
    ret

; We now have separate, hardcoded functions for each interrupt needed.
; This is far more stable than the previous self-modifying version.

_interrupt10:
    push bp
    mov bp, sp
    mov ax, [bp+4]
    mov bx, [bp+6]
    mov cx, [bp+8]
    mov dx, [bp+10]
    int 0x10
    mov [bp+4], ax ; Return value in AX
    pop bp
    ret

_interrupt13:
    push bp
    mov bp, sp
    mov ax, [bp+4]
    mov bx, [bp+6]
    mov cx, [bp+8]
    mov dx, [bp+10]
    int 0x13
    mov [bp+4], ax
    pop bp
    ret

_interrupt16:
    push bp
    mov bp, sp
    mov ax, [bp+4]
    mov bx, [bp+6]
    mov cx, [bp+8]
    mov dx, [bp+10]
    int 0x16
    mov [bp+4], ax
    pop bp
    ret

_interrupt21:
    push bp
    mov bp, sp
    mov ax, [bp+4]
    mov bx, [bp+6]
    mov cx, [bp+8]
    mov dx, [bp+10]
    int 0x21
    mov [bp+4], ax
    pop bp
    ret