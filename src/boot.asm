[org 0x7c00]
bits 16

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; --- 1. LOAD IMAGE ---
    mov ax, 0x1000
    mov es, ax
    xor bx, bx
    mov ax, 0x0220   ; ah=0x02 (Read), al=32 (sectors)
    mov cx, 0x0002   ; ch=0 (Cyl 0), cl=2 (Sector 2)
    mov dh, 0        ; dh=0 (Head 0)
    int 0x13
    jc error

    ; --- 2. SET MODE 13h ---
    mov ax, 0x0013
    int 0x10

    ; --- 3. CUSTOMIZE RAIN COLORS TO BLOOD RED ---
    mov dx, 0x03C8
    mov al, 40       ; Index 40 (Rain Head)
    out dx, al
    inc dx           ; Port 0x03C9 (Data)
    mov al, 45       ; Red channel
    out dx, al
    xor al, al       ; Green & Blue (0)
    out dx, al
    out dx, al

    dec dx           ; Port 0x03C8
    mov al, 42       ; Index 42 (Rain Trail)
    out dx, al
    inc dx           ; Port 0x03C9
    mov al, 18       ; Dark Red
    out dx, al
    xor al, al
    out dx, al
    out dx, al

    ; --- 4. DRAW IMAGE (Optimized 128x128 loop) ---
    mov ax, 0xA000
    mov es, ax
    mov ax, 0x1000
    mov ds, ax
    xor si, si
    mov di, 36 * 320 + 96 ; Start position (Y=36, X=96)
    mov dx, 128      ; 128 rows
draw_row:
    mov cx, 64       ; 128 bytes per row = 64 words
    rep movsw
    add di, 320 - 128; Jump to next row
    dec dx
    jnz draw_row

    ; --- 5. PRINT TEXT RELIABLY ---
    xor ax, ax
    mov ds, ax
    mov es, ax

    mov ah, 0x02
    mov bh, 0
    mov dx, 0x0209
    int 0x10

    mov si, msg
print_loop:
    lodsb
    cmp al, 0
    je print_done
    mov ah, 0x0E
    mov bl, 0x04
    int 0x10
    jmp print_loop
print_done:

; --- MAIN LOOP ---
hang:
    ; 1. Jitter Effect
    mov dx, 0x03D4
    mov al, 0x0D
    out dx, al
    inc dx
    in al, dx
    xor al, 1
    out dx, al

    ; 2. Play Megalovania Note
    call play_next_note

    ; 3. Scary Image Glitch
    call glitch_image

    ; 4. Realistic Blood Rain Effect
    call blood_rain_effect

    ; 5. Delay ~33ms (30 FPS)
    mov ah, 0x86
    xor cx, cx
    mov dx, 0x80E8
    int 0x15

    jmp hang

; --- LFSR RANDOM NUMBER GENERATOR ---
get_random:
    mov ax, [random_seed]
    mov dx, ax
    shr ax, 2
    xor ax, dx
    and ax, 1
    shl ax, 15
    shr word [random_seed], 1
    or [random_seed], ax
    mov ax, [random_seed]
    ret

; --- SCARY IMAGE GLITCH ---
glitch_image:
    call get_random
    and ax, 127
    add ax, 36
    imul ax, 320
    add ax, 96
    mov di, ax

    call get_random
    and ax, 15
    mov si, di
    add si, ax

    mov cx, 64       ; 128 bytes = 64 words

    push ds
    push 0xA000
    pop ds
    push 0xA000
    pop es
    rep movsw
    pop ds
    ret

; --- REALISTIC BLOOD RAIN EFFECT ---
blood_rain_effect:
    push ds
    push 0xA000
    pop ds
    push 0xA000
    pop es

    mov si, 64000 - 640 - 1
.fall_loop:
    mov al, [si]
    cmp al, 40
    je .move_head
    cmp al, 42
    je .fade_trail
    jmp .next_pixel

.move_head:
    mov byte [si+640], 40
    mov byte [si+320], 42
    mov byte [si], 0
    jmp .next_pixel

.fade_trail:
    mov byte [si], 0

.next_pixel:
    dec si
    cmp si, 0xFFFF
    jne .fall_loop

    mov cx, 4
.spawn_loop:
    call get_random
    xor dx, dx
    mov bx, 320
    div bx
    mov di, dx
    mov byte [di], 40
    loop .spawn_loop

    pop ds
    ret

; --- COMPRESSED MEGALOVANIA PLAYER ---
play_next_note:
    cmp byte [frame_wait], 0
    ja .dec_wait

    ; Fetch next byte from sequence
    mov si, [note_ptr]
    lodsb
    cmp al, 0xFF       ; Loop marker?
    jne .parse
    mov si, mega_seq
    lodsb
.parse:
    mov [note_ptr], si

    ; Extract Duration (Lo-Nibble)
    mov bl, al
    and bl, 0x0F
    mov [frame_wait], bl

    ; Extract Note Index (Hi-Nibble) and lookup Frequency
    shr al, 4
    cbw                ; Zero-extend AL to AX (since max index is 9)
    shl ax, 1          ; Multiply by 2 (Word array)
    mov bx, ax
    mov ax, [freq_table + bx]

    test ax, ax        ; Is it a rest?
    jz .rest

    ; Enable Speaker & Send Pitch
    push ax
    in al, 0x61
    or al, 0x03
    out 0x61, al
    pop ax

    mov bx, ax
    mov al, 0xB6
    out 0x43, al
    mov ax, bx
    out 0x42, al
    mov al, ah
    out 0x42, al
    ret

.rest:
    in al, 0x61
    and al, 0xFC
    out 0x61, al
    ret

.dec_wait:
    dec byte [frame_wait]
    ret

error:
    mov ah, 0x0e
    mov al, 'E'
    int 0x10
    jmp $

; --- DATA SECTION ---
random_seed dw 0xACE1
msg db "I'M STILL HERE", 0
note_ptr dw mega_seq
frame_wait db 0

; Note Frequency Lookup Table (0 = Rest)
freq_table:
    dw 0, 4063, 4560, 4831, 5119, 2031, 2711, 2873, 3043, 3416

; Compressed Megalovania (Hi-Nibble = Note Index, Lo-Nibble = Duration)
mega_seq:
    db 0x13, 0x01, 0x13, 0x01 ; Phrase 1 Head (D4)
    db 0x56, 0x02, 0x68, 0x04, 0x74, 0x01, 0x84, 0x01, 0x96, 0x02, 0x13, 0x01, 0x93, 0x01, 0x83, 0x05 ; Tail

    db 0x23, 0x01, 0x23, 0x01 ; Phrase 2 Head (C4)
    db 0x56, 0x02, 0x68, 0x04, 0x74, 0x01, 0x84, 0x01, 0x96, 0x02, 0x13, 0x01, 0x93, 0x01, 0x83, 0x05

    db 0x33, 0x01, 0x33, 0x01 ; Phrase 3 Head (B3)
    db 0x56, 0x02, 0x68, 0x04, 0x74, 0x01, 0x84, 0x01, 0x96, 0x02, 0x13, 0x01, 0x93, 0x01, 0x83, 0x05

    db 0x43, 0x01, 0x43, 0x01 ; Phrase 4 Head (Bb3)
    db 0x56, 0x02, 0x68, 0x04, 0x74, 0x01, 0x84, 0x01, 0x96, 0x02, 0x13, 0x01, 0x93, 0x01, 0x83, 0x05
    db 0xFF ; Loop marker

times 510-($-$$) db 0
dw 0xAA55