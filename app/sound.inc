; SPDX-License-Identifier: MIT
; sound.inc - page "0 Klang": the PC speaker.
;
; GeoWorks 1 has no sound library for applications (it came with
; GeoWorks 2), so the page drives the speaker itself: timer 2 (ports 42h,
; 43h) as a square wave, switched on and off with port 61h; durations in
; ticks of 1/60 s with TimerSleep (kernel 184).
;
; Playing does not block the programme: every note is one message 0C06h
; to our process, sent with MF_FORCE_QUEUE (4000h, as UI.GEO uses it), so
; keys and clicks are handled between the notes. If the message came back
; at once instead of through the queue, the loop in snd_step goes on.

%define KB_H      26
%define KL_T1     (ROW_Y+124)     ; title "Abspielen"
%define KL_R1     (ROW_Y+146)     ; first row of buttons
%define KL_T2     (ROW_Y+184)
%define KL_R2     (ROW_Y+206)
%define KL_ST     (ROW_Y+246)     ; status line
%define MF_FORCE_QUEUE 0x4000
%define MSG_SND_STEP   0x0c06

; divisors of the timer (1193182 Hz / frequency): C4 D4 E4 F4 G4 A4 B4 C5
notes_div: dw 4560,4063,3620,3417,3044,2712,2416,2280

songs:       dw song_a,song_scale,song_duck,song_chord
song_names:  dw k_b0,k_b1,k_b2,k_b3
instr_names: dw k_i0,k_i1,k_i2,k_i3

; note (0-7 = C4..C5, +80h = tied to the next one), length in ticks; FFh end
song_a:     db 5,60, 0xff
song_scale: db 0,15, 1,15, 2,15, 3,15, 4,15, 5,15, 6,15, 7,30, 0xff
; Alle meine Entchen (folk song), an eighth = 7 ticks
song_duck:  db 0,14, 1,14, 2,14, 3,14, 4,28, 4,28
            db 5,14, 5,14, 5,14, 5,14, 4,56
            db 5,14, 5,14, 5,14, 5,14, 4,56
            db 3,14, 3,14, 3,14, 3,14, 2,28, 2,28
            db 1,14, 1,14, 1,14, 1,14, 0,56, 0xff
; C major chord: one voice only, so C-E-G as a fast arpeggio for 1.5 s
song_chord: times 10 db 0x80,3, 0x82,3, 0x84,3
            db 0xff
song_stop:  db 0xff
s_kdash:    db ' ',0xd0,' ',0
s_kplaying: db 'Spielt: ',0
s_kready:   db 'Instrument: ',0
s_kready2:  db ' ',0xd0,' einen Knopf der oberen Reihe anklicken',0

; ----------------------------------------------------------------------
; speaker
; ----------------------------------------------------------------------
spk_set:                          ; DX = divisor: tone on
 push ax
 pushf
 cli
 mov al,0xb6                      ; timer 2, lo/hi byte, square wave
 out 0x43,al
 mov al,dl
 out 0x42,al
 mov al,dh
 out 0x42,al
 in al,0x61
 or al,3                          ; gate and speaker on
 out 0x61,al
 popf
 pop ax
 ret
spk_off:
 push ax
 pushf
 cli
 in al,0x61
 and al,0xfc
 out 0x61,al
 popf
 pop ax
 ret
snd_sleep:                        ; AX ticks
 or ax,ax
 jz .r
 push ax
 push bx
 push cx
 push dx
 push si
 push di
 push bp
 push es
 KCALL 184                        ; TimerSleep
 pop es
 pop bp
 pop di
 pop si
 pop dx
 pop cx
 pop bx
 pop ax
.r:
 ret

; AL = note (+80h tied), AH = length in ticks. The instruments differ
; only in their touch: piano 3/4 sounding, organ legato, flute with
; vibrato, xylophone an octave higher and short.
play_note:
 mov bl,al
 and bx,7
 shl bx,1
 mov dx,[cs:notes_div+bx]
 mov cl,ah
 xor ch,ch                        ; CX = length
 xor bp,bp                        ; BP = rest at the end
 mov es,[V_LSEG]
 mov bx,[es:L_INSTR]
 cmp bx,3
 jne .i0
 shr dx,1                         ; xylophone: an octave higher
.i0:
 test al,0x80
 jnz .play                        ; tied: no rest
 or bx,bx
 jnz .i1
 mov bp,cx                        ; piano: a quarter rest
 shr bp,1
 shr bp,1
 jmp .play
.i1:
 cmp bx,1
 jne .i2
 cmp cx,2                         ; organ: one tick
 jbe .play
 mov bp,1
 jmp .play
.i2:
 cmp bx,2
 jne .i3
 mov bp,cx                        ; flute: an eighth
 shr bp,1
 shr bp,1
 shr bp,1
 jmp .play
.i3:
 mov bp,cx                        ; xylophone: 4 ticks sound
 sub bp,4
 jnc .play
 xor bp,bp
.play:
 sub cx,bp                        ; CX = sounding ticks
 call spk_set
 cmp bx,2
 jne .plain
 test al,0x80
 jnz .plain
 ; vibrato: every 2 ticks a little higher and back
 mov si,dx
 mov ax,dx
 push cx
 mov cl,6
 shr ax,cl
 pop cx
 sub si,ax                        ; SI = higher divisor
.vib:
 jcxz .rest
 mov ax,2
 cmp cx,ax
 jae .v1
 mov ax,cx
.v1:
 sub cx,ax
 call snd_sleep
 xchg dx,si
 call spk_set
 jmp .vib
.plain:
 mov ax,cx
 call snd_sleep
.rest:
 or bp,bp
 jz .r
 call spk_off
 mov ax,bp
 call snd_sleep
.r:
 ret

; ----------------------------------------------------------------------
; playing: one note per message
; ----------------------------------------------------------------------
queue_step:
 push ax
 push bx
 push cx
 push dx
 push si
 push di
 push bp
 push es
 KCALL 149                        ; GeodeGetProcessHandle -> BX
 xor si,si
 mov ax,MSG_SND_STEP
 mov di,MF_FORCE_QUEUE
 KCALL 169                        ; ObjMessage
 pop es
 pop bp
 pop di
 pop si
 pop dx
 pop cx
 pop bx
 pop ax
 ret

; process method 0C06h: the next note
snd_step:
 cmp word [V_INIT],0
 jne .go
 call spk_off
 retf
.go:
 mov es,[V_LSEG]
 cmp word [es:L_SNDBUSY],0
 je .run
 mov word [es:L_SNDRE],1          ; came back at once: the loop goes on
 retf
.run:
 mov word [es:L_SNDBUSY],1
.next:
 mov es,[V_LSEG]
 mov word [es:L_SNDRE],0
 mov si,[es:L_SNDP]
 mov ax,[cs:si]
 cmp al,0xff
 je .end
 add si,2
 mov [es:L_SNDP],si
 push di
 push bp
 call play_note
 pop bp
 pop di
 mov es,[V_LSEG]
 mov si,[es:L_SNDP]
 cmp byte [cs:si],0xff
 je .end
 call queue_step
 mov es,[V_LSEG]
 cmp word [es:L_SNDRE],0
 jne .next
 mov word [es:L_SNDBUSY],0
 retf
.end:
 call spk_off
 mov es,[V_LSEG]
 mov word [es:L_SNDRUN],0
 mov word [es:L_SNDBUSY],0
 call sound_refresh
 retf

; AX = song 0-3
start_song:
 mov es,[V_LSEG]
 mov [es:L_SONG],ax
 mov bx,ax
 shl bx,1
 mov ax,[cs:songs+bx]
 mov [es:L_SNDP],ax
 cmp word [es:L_SNDRUN],0
 jne .r
 mov word [es:L_SNDRUN],1
 call queue_step
.r:
 ret

; leaving the page or the programme: the melody ends after this note
stop_song:
 cmp word [V_INIT],0
 je .r
 mov es,[V_LSEG]
 cmp word [es:L_SNDRUN],0
 je .r
 mov word [es:L_SNDP],song_stop
.r:
 ret

; ----------------------------------------------------------------------
; the page: rows of the report (page 9), buttons, status
; ----------------------------------------------------------------------
; AX = button width for the window width
kb_width:
 mov ax,[V_VW]
 sub ax,36
 shr ax,1
 shr ax,1
 sub ax,8
 cmp ax,90
 jge .a
 mov ax,90
.a:
 cmp ax,146
 jle .b
 mov ax,146
.b:
 ret

draw_sound:
 mov cx,F_UI
 mov dx,9
 call set_font
 mov ax,0xff00
 call set_style
 mov al,C_DGREY
 call set_tcol
 mov si,s_khint1
 mov ax,24
 mov bx,ROW_Y+88
 call text_cs
 mov si,s_khint2
 mov bx,ROW_Y+101
 call text_cs
 ; titles
 mov cx,F_UI
 mov dx,12
 call set_font
 mov ax,0x0020
 call set_style
 mov al,C_DBLUE
 call set_tcol
 mov si,s_kplay
 mov ax,18
 mov bx,KL_T1
 call text_cs
 mov si,s_kinstr
 mov bx,KL_T2
 call text_cs
 call draw_sound_ui
 ret

; buttons and status (also while playing)
draw_sound_ui:
 xor cx,cx                        ; row
.row:
 xor si,si                        ; button
.btn:
 push cx
 push si
 call draw_kbutton
 pop si
 pop cx
 inc si
 cmp si,4
 jb .btn
 inc cx
 cmp cx,2
 jb .row
 ; status
 mov al,C_WHITE
 call set_area
 mov ax,14
 mov bx,KL_ST-2
 mov cx,[V_VW]
 sub cx,14
 mov dx,KL_ST+16
 call fill
 mov cx,F_UI
 mov dx,10
 call set_font
 mov ax,0x0020
 call set_style
 mov al,C_DBLUE
 call set_tcol
 call v_reset
 mov es,[V_LSEG]
 cmp word [es:L_SNDRUN],0
 je .idle
 VS s_kplaying
 mov bx,[es:L_SONG]
 shl bx,1
 mov si,[cs:song_names+bx]
 call v_s
 VS s_kdash
 mov bx,[es:L_INSTR]
 shl bx,1
 mov si,[cs:instr_names+bx]
 call v_s
 jmp .show
.idle:
 VS s_kready
 mov bx,[es:L_INSTR]
 shl bx,1
 mov si,[cs:instr_names+bx]
 call v_s
 VS s_kready2
.show:
 mov si,L_VAL
 mov ax,22
 mov bx,KL_ST
 call text_ls
 mov ax,0xff00
 call set_style
 ret

; CX = row (0 play, 1 instrument), SI = button
draw_kbutton:
 call kb_rect                     ; -> AX,BX,CX,DX
 push cx
 push si
 mov si,C_LBLUE
 mov es,[V_LSEG]
 cmp word [V_KROW],0              ; (set by kb_rect)
 jne .ins
 cmp word [es:L_SNDRUN],0
 je .col
 mov si,[V_KBTN]
 cmp si,[es:L_SONG]
 mov si,C_LBLUE
 jne .col
 mov si,C_DBLUE
 jmp .col
.ins:
 mov si,[V_KBTN]
 cmp si,[es:L_INSTR]
 mov si,C_LBLUE
 jne .col
 mov si,C_DBLUE
.col:
 push ax
 mov ax,si
 call set_area
 pop ax
 pop si
 pop cx
 call fill
 push ax
 mov al,C_DGREY
 call set_lcol
 pop ax
 call frame
 add ax,8
 add bx,7
 push ax
 push bx
 mov cx,F_UI
 mov dx,10
 call set_font
 mov ax,0x0020
 call set_style
 mov al,C_WHITE
 call set_tcol
 mov bx,[V_KBTN]
 shl bx,1
 mov si,[cs:song_names+bx]
 cmp word [V_KROW],0
 je .t
 mov si,[cs:instr_names+bx]
.t:
 pop bx
 pop ax
 call text_cs
 mov ax,0xff00
 call set_style
 ret

; CX = row, SI = button -> AX,BX,CX,DX rectangle (and V_KROW, V_KBTN)
kb_rect:
 mov [V_KROW],cx
 mov [V_KBTN],si
 call kb_width
 mov cx,ax
 add ax,8
 mul si
 add ax,18
 mov bx,KL_R1
 cmp word [V_KROW],0
 je .y
 mov bx,KL_R2
.y:
 add cx,ax
 mov dx,bx
 add dx,KB_H
 ret

; click at CX,DX (window) on the sound page
sound_click:
 push cx
 push dx
 mov [V_KX],cx
 mov [V_KY],dx
 xor cx,cx
.row:
 xor si,si
.btn:
 push cx
 push si
 call kb_rect
 cmp [V_KX],ax
 jl .no
 cmp [V_KX],cx
 jg .no
 cmp [V_KY],bx
 jl .no
 cmp [V_KY],dx
 jg .no
 pop si
 pop cx
 jmp .hit
.no:
 pop si
 pop cx
 inc si
 cmp si,4
 jb .btn
 inc cx
 cmp cx,2
 jb .row
 pop dx
 pop cx
 ret
.hit:
 or cx,cx
 jnz .instr
 mov ax,si
 call start_song
 jmp .redraw
.instr:
 mov es,[V_LSEG]
 mov [es:L_INSTR],si
.redraw:
 call sound_refresh
 pop dx
 pop cx
 ret

; buttons and status again, if the page is shown
sound_refresh:
 cmp word [V_PAGE],PG_KLANG
 jne .r
 call begin_direct
 jc .r
 call draw_sound_ui
 call end_direct
.r:
 ret

; report rows of the sound page (page 9) and the number of fonts (page 4)
sound_rows:
 mov al,PG_KLANG
 call r_page
 HEAD h_sound
 VS v_sndout
 ROW l_sndout
 INIROW l_uisnd,s_sui,s_ksound
 mov si,H_BLASTER
 call v_ls_or
 ROW l_blaster
 VS v_sndlib
 ROW l_sndlib
 mov al,4
 call r_page
 mov es,[V_LSEG]
 mov ax,[es:L_NFONT]
 call v_n
 call v_sp
 VS v_fntfiles
 ROW l_fonts
 ret
s_ksound: db 'sound',0
