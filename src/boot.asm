[org 0x7c00]
bits 16

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; Load image
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

    ; Set Mode 13h
    mov ax, 0x0013
    int 0x10

    ; Draw Image
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

    ; --- PRINT TEXT RELIABLY ---
    xor ax, ax
    mov ds, ax
    mov es, ax

    mov ah, 0x02
    mov bh, 0
    mov dx, 0x0210
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

    ; --- INITIALIZE SPEAKER ONCE ---
    in al, 0x61
    or al, 0x03
    out 0x61, al

; --- MAIN LOOP ---
hang:
    ; 1. Jitter Effect (Hardware Level)
    mov dx, 0x03D4
    mov al, 0x0D
    out dx, al
    inc dx
    in al, dx
    xor al, 1
    out dx, al

    ; 2. Play Dark Atmosphere Audio
    call play_next_note

    ; 3. Scary Image Glitch (VRAM Tearing)
    call glitch_image

    ; 4. Realistic Blood Rain Effect
    call blood_rain_effect

    ; 5. Reliable BIOS Time Delay (Changed to 33ms for smooth 30 FPS rain)
    mov ah, 0x86
    mov cx, 0x0000
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

; --- SCARY IMAGE GLITCH (HORIZONTAL TEARING) ---
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

    mov cx, 128

    push ds
    mov ax, 0xA000
    mov ds, ax
    mov es, ax
    rep movsb
    pop ds
    ret

; --- REALISTIC BLOOD RAIN EFFECT ---
blood_rain_effect:
    push ds
    mov ax, 0xA000
    mov ds, ax
    mov es, ax
    
    ; Scan VRAM from bottom to top, stopping 2 rows before the end
    mov si, 64000 - 640 - 1
.fall_loop:
    mov al, [si]
    cmp al, 40       ; Color 40 = Bright Red (Drop Head)
    je .move_head
    cmp al, 42       ; Color 42 = Darker Red (Drop Trail)
    je .fade_trail
    jmp .next_pixel

.move_head:
    mov byte [si+640], 40 ; Move head down 2 rows
    mov byte [si+320], 42 ; Leave a darker trail behind it
    mov byte [si], 0      ; Clear the old head to black
    jmp .next_pixel

.fade_trail:
    mov byte [si], 0      ; Erase the old trail to black so it doesn't smear forever

.next_pixel:
    dec si
    cmp si, 0xFFFF        ; <--- FIX: Properly bounds check without relying on signed flags
    jne .fall_loop

    ; Spawn new blood drops at random X coordinates at the top of the screen
    mov cx, 4             ; Spawn 4 new drops per frame
.spawn_loop:
    call get_random
    xor dx, dx
    mov bx, 320
    div bx                ; Remainder in DX = random X coordinate (0-319)
    mov di, dx
    mov byte [di], 40     ; Spawn new bright red drop head
    loop .spawn_loop

    pop ds
    ret

; --- DARK ATMOSPHERE SOUND ROUTINE ---
play_next_note:
    inc byte [sound_state]
    test byte [sound_state], 0x80  ; Adjusted for 33ms loop (~4.2 sec per state)
    jz .drone_state

.scream_state:
    ; Emulate a descending shriek: Pitch drops rapidly (divisor increases)
    call get_random
    and ax, 0x01FF       ; Random variance for chaotic feel
    add [pitch_val], ax  ; Drop the pitch
    add word [pitch_val], 200 ; Constant downward pull
    cmp word [pitch_val], 25000 ; Did it hit the bottom threshold?
    jl .apply
    mov word [pitch_val], 1500 ; Reset to a high frequency to "scream" again
    jmp .apply

.drone_state:
    ; Emulate a hopeless, heavy, low rumble
    call get_random
    and ax, 0x0FFF       ; Heavy variance for instability
    add ax, 15000        ; Base deep rumble (very low Hz)
    mov [pitch_val], ax

.apply:
    mov ax, [pitch_val]
    ; Send pitch to PIT Channel 2
    mov bx, ax
    mov al, 0xB6
    out 0x43, al
    mov ax, bx
    out 0x42, al
    mov al, ah
    out 0x42, al
    ret

; --- DATA SECTION ---
random_seed dw 0xACE1
sound_state db 0
pitch_val dw 2000

msg db "I'M STILL HERE", 0

error:
    mov ah, 0x0e
    mov al, 'E'
    int 0x10
    jmp $

times 446-($-$$) db 0
times 510-($-$$) db 0
dw 0xAA55