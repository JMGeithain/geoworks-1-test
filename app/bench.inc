; SPDX-License-Identifier: MIT
; bench.inc - performance page: processor, graphics, memory and file tests,
; each counted for about half a second against the BIOS clock.

%define B_TICKS 9                 ; ticks per test (about 0.5 s)

do_measure:
 cmp word [V_INIT],0
 jne .go
 ret
.go:
 push bp
 call .run
 pop bp
 ret
.run:
 mov word [V_PAGE],5
 mov word [V_SCR],0
 mov word [V_MSG],m_meas
 call repaint
 call cpu_speed
 call begin_direct
 jc .nogfx
 ; test area: the panel below the processor rows
 mov es,[V_LSEG]
 mov word [es:L_PADX],24
 mov word [es:L_PADY],ROW_Y+60
 mov ax,[V_VW]
 sub ax,24+24+48
 cmp ax,100
 jge .w
 mov ax,100
.w:
 mov [es:L_PADW],ax
 mov ax,[V_VH]
 sub ax,ROW_Y+60+FOOT_H+40
 cmp ax,60
 jge .h
 mov ax,60
.h:
 mov [es:L_PADH],ax
 mov al,C_WHITE
 call set_area
 mov ax,[es:L_PADX]
 mov bx,[es:L_PADY]
 mov cx,ax
 add cx,[es:L_PADW]
 add cx,48
 mov dx,bx
 add dx,[es:L_PADH]
 add dx,30
 call fill
 mov cx,F_UI
 mov dx,12
 call set_font
 mov bp,op_rect
 call b_run
 mov es,[V_LSEG]
 mov [es:B_RECT],ax
 mov bp,op_text
 call b_run
 mov es,[V_LSEG]
 mov [es:B_TEXT],ax
 mov bp,op_ell
 call b_run
 mov es,[V_LSEG]
 mov [es:B_ELL],ax
 mov bp,op_bmp
 call b_run
 mov es,[V_LSEG]
 mov [es:B_BMP],ax
 call end_direct
.nogfx:
 mov bp,op_mem
 call b_run
 mov es,[V_LSEG]
 mov [es:B_MEM],ax
 call b_file
 mov es,[V_LSEG]
 mov word [es:H_BENCH],1
 mov word [V_MSG],m_done
 call repaint
 ret

; BP = operation (DI = gstate, may use AX BX CX DX ES) -> AX = per second
b_run:
 push si
 push di
 call tick_sync
 jnc .ok
 xor ax,ax
 jmp .r
.ok:
 mov es,[V_LSEG]
 mov [es:L_TICK],bx
 xor si,si
.l:
 push bp
 push si
 push di
 call bp
 pop di
 pop si
 pop bp
 inc si
 cmp si,0xfff0
 jae .stop
 mov es,[V_LSEG]
 mov bx,[es:L_TICK]
 call tick_since
 cmp ax,B_TICKS
 jb .l
.stop:
 mov es,[V_LSEG]
 mov bx,[es:L_TICK]
 call tick_since
 mov cx,ax
 or cx,cx
 jnz .t
 inc cx
.t:
 mov ax,10
 mul cx
 mov cx,ax                        ; ticks * 10
 mov ax,si
 mov bx,182
 mul bx                           ; ops * 182
 call div_sat
.r:
 pop di
 pop si
 ret

; DX:AX / CX, 65535 when the result does not fit
div_sat:
 cmp dx,cx
 jae .big
 div cx
 ret
.big:
 mov ax,0xffff
 ret

; AX = random number 0..CX-1
rnd:
 push dx
 mov ax,[V_SEED]
 mov dx,25173
 mul dx
 add ax,13849
 mov [V_SEED],ax
 mul cx
 mov ax,dx
 pop dx
 ret

; random position in the test area for a CX x DX object -> AX, BX
rnd_pos:
 push cx
 push dx
 mov es,[V_LSEG]
 push dx
 mov ax,[es:L_PADW]
 sub ax,cx
 mov cx,ax
 call rnd
 add ax,[es:L_PADX]
 mov bx,ax
 pop dx
 mov cx,[es:L_PADH]
 sub cx,dx
 call rnd
 add ax,[es:L_PADY]
 xchg ax,bx
 pop dx
 pop cx
 ret

op_rect:
 mov cx,30
 mov dx,18
 call rnd_pos
 push ax
 mov ax,si
 and al,15
 call set_area
 pop ax
 mov cx,ax
 add cx,30
 mov dx,bx
 add dx,18
 call fill
 ret

op_text:
 mov cx,70
 mov dx,16
 call rnd_pos
 push ax
 mov ax,si
 and al,15
 call set_tcol
 pop ax
 mov si,s_bench
 call text_cs
 ret

op_ell:
 mov cx,40
 mov dx,26
 call rnd_pos
 push ax
 mov ax,si
 and al,15
 call set_lcol
 pop ax
 mov cx,ax
 add cx,40
 mov dx,bx
 add dx,26
 call ellipse
 ret

op_bmp:
 mov cx,48
 mov dx,30
 call rnd_pos
 call draw_icon_at
 ret

op_mem:
 mov ax,1024
 mov cx,0xc000
 KCALL 0                          ; MemAlloc
 jc .r
 KCALL 2                          ; MemFree
.r:
 ret

; current folder = GEOWORKS\STATE (like Schnecke and GeoMaler)
goto_state:
 mov ax,0
 KCALL 76                         ; FileSetStandardPath SP_TOP
 push ds
 push cs
 pop ds
 mov bx,0
 mov dx,s_statedir
 KCALL 51                         ; FileSetCurrentPath
 pop ds
 ret

; write / read 8 KB blocks of STATE\GWTEST.TMP; the file is emptied again
%define FBLK 8192
%define FMAX 128
b_file:
 mov es,[V_LSEG]
 mov word [es:B_FOK],0
 KCALL 73                         ; FilePushDir
 call goto_state
 jnc .cr
 jmp .pop
.cr:
 push ds
 push cs
 pop ds
 mov dx,s_tmpname
 mov ax,0x00c1                    ; create/truncate, deny none, write+read
 xor cx,cx
 KCALL 53                         ; FileCreate
 pop ds
 jnc .op
 jmp .pop
.op:
 mov es,[V_LSEG]
 mov [es:L_FH],ax
 call tick_sync
 jnc .t0
 jmp .close
.t0:
 mov [es:L_TICK],bx
 xor si,si
.wr:
 mov es,[V_LSEG]
 mov bx,[es:L_FH]
 mov cx,FBLK
 push ds
 mov ds,[V_LSEG]
 mov dx,L_TXT
 mov al,0
 KCALL 59                         ; FileWrite
 pop ds
 jc .close
 inc si
 cmp si,FMAX
 jae .wd
 mov es,[V_LSEG]
 mov bx,[es:L_TICK]
 call tick_since
 cmp ax,B_TICKS
 jb .wr
.wd:
 call f_rate
 mov es,[V_LSEG]
 mov [es:B_WR],ax
 ; read it back: close, open again for reading (as GEOS.INI is read)
 mov [es:L_CNT],si
 mov bx,[es:L_FH]
 mov al,0
 KCALL 54                         ; FileClose
 push ds
 push cs
 pop ds
 mov dx,s_tmpname
 mov al,0x40                      ; read, deny none
 KCALL 52                         ; FileOpen
 pop ds
 jnc .ro
 jmp .trunc
.ro:
 mov es,[V_LSEG]
 mov [es:L_FH],ax
 call tick_sync
 mov es,[V_LSEG]
 mov [es:L_TICK],bx
 xor si,si
.rd:
 mov es,[V_LSEG]
 mov bx,[es:L_FH]
 mov cx,FBLK
 push ds
 mov ds,[V_LSEG]
 mov dx,L_TXT
 mov al,0
 KCALL 58                         ; FileRead
 pop ds
 jc .rdone
 inc si
 mov es,[V_LSEG]
 cmp si,[es:L_CNT]
 jae .rdone
 mov bx,[es:L_TICK]
 call tick_since
 cmp ax,B_TICKS
 jb .rd
.rdone:
 call f_rate
 mov es,[V_LSEG]
 mov [es:B_RD],ax
 mov word [es:B_FOK],1
.close:
 mov es,[V_LSEG]
 mov bx,[es:L_FH]
 mov al,0
 KCALL 54                         ; FileClose
.trunc:                           ; empty the file again
 push ds
 push cs
 pop ds
 mov dx,s_tmpname
 mov ax,0x00c1
 xor cx,cx
 KCALL 53
 pop ds
 jc .pop
 mov bx,ax
 mov al,0
 KCALL 54
.pop:
 KCALL 74                         ; FilePopDir
 ret

; SI blocks of 8 KB since L_TICK -> AX = KB/s
f_rate:
 mov es,[V_LSEG]
 mov bx,[es:L_TICK]
 call tick_since
 mov cx,ax
 or cx,cx
 jnz .t
 inc cx
.t:
 mov ax,10
 mul cx
 mov cx,ax
 mov ax,si
 mov bx,8*182
 mul bx
 call div_sat
 ret
