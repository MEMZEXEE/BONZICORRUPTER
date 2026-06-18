[org 0x7c00]
bits 16

start:
    ; Setup segments
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; STEP 1: Load image data from disk
    mov ax, 0x1000
    mov es, ax
    xor bx, bx
    mov ah, 0x02
    mov al, 32
    mov ch, 0
    mov cl, 2
    mov dh, 0
    int 0x13
    jc error

    ; STEP 2: Set VGA Mode 13h
    mov ax, 0x0013
    int 0x10

    ; STEP 3: Draw the Image
    mov ax, 0xA000
    mov es, ax
    mov ax, 0x1000
    mov ds, ax
    xor si, si
    mov dx, 36

draw_y:
    mov bx, 96
draw_x:
    mov ax, dx
    imul ax, 320
    add ax, bx
    mov di, ax
    movsb
    inc bx
    cmp bx, 224
    jne draw_x
    inc dx
    cmp dx, 164
    jne draw_y

    ; STEP 4: Print "I'M STILL HERE"
    xor ax, ax
    mov ds, ax
    
    mov ah, 0x02
    mov bh, 0
    mov dx, 0x0208 
    int 0x10

    mov si, msg
    call print_string

; --- MOVING JITTER EFFECT ---
hang:
    ; Apply simple screen jitter
    mov dx, 0x03D4      ; VGA CRTC Index Register
    mov al, 0x0D        ; Start Address Low Register
    out dx, al
    inc dx
    in al, dx
    xor al, 1           ; Toggle last bit for jitter
    out dx, al
    
    ; Delay loop
    mov cx, 0x7FFF
.delay:
    loop .delay
    jmp hang

; Subroutine to print string in RED
print_string:
.loop:
    mov ah, 0x09      ; MUST reset AH to 0x09 every iteration
    mov bl, 0x04      ; Red color
    mov cx, 1         ; Print one char
    
    lodsb             ; Get char
    cmp al, 0
    je .done
    
    int 0x10          ; Print
    
    ; Move cursor
    mov ah, 0x03
    int 0x10
    inc dl
    mov ah, 0x02
    int 0x10
    
    jmp .loop
.done:
    ret

msg db "I'M STILL HERE", 0

error:
    mov ah, 0x0e
    mov al, 'E'
    int 0x10
    jmp hang

times 446-($-$$) db 0
times 510-($-$$) db 0
dw 0xAA55