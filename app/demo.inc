; SPDX-License-Identifier: MIT
; demo.inc - page "Grafik" (what the kernel graphics calls can do) and
; page "Eingabe" (keyboard and mouse events).

%define GX0  24                   ; left column
%define GX1  340                  ; right column

; CS:SI title at AX,BX -> BX below it
g_title:
 push ax
 mov cx,F_UI
 mov dx,12
 call set_font
 mov ax,0x0020
 call set_style
 mov al,C_DBLUE
 call set_tcol
 pop ax
 call text_cs
 push ax
 mov ax,0xff00
 call set_style
 pop ax
 add bx,17
 ret

draw_graphics:
 ; ---- 16 colours ----
 mov si,g_pal
 mov ax,GX0
 mov bx,ROW_Y
 call g_title
 mov cx,F_UI
 mov dx,9
 call set_font
 xor si,si
.pal:
 mov ax,si
 call set_area
 mov ax,si
 mov cl,4
 mov dx,18
 mul dx
 add ax,GX0
 mov cx,ax
 add cx,16
 mov dx,bx
 add dx,18
 call fill
 push ax
 mov al,C_DGREY
 call set_lcol
 pop ax
 call frame
 ; index below
 push ax
 push bx
 add bx,20
 mov al,C_BLACK
 call set_tcol
 call v_reset
 mov ax,si
 call v_n
 pop bx
 pop ax
 push bx
 add bx,19
 push si
 mov si,L_VAL
 call text_ls
 pop si
 pop bx
 inc si
 cmp si,16
 jb .pal
 add bx,36
 ; ---- RGB ramps ----
 mov si,g_rgb
 mov ax,GX0
 call g_title
 xor cx,cx                        ; ramp 0 red, 1 green, 2 blue, 3 grey
.ramp:
 xor si,si
.step:
 push cx
 push bx
 mov ax,si
 shl ax,1
 shl ax,1                         ; 0..252
 mov ah,al
 xor al,al
 xor bx,bx
 cmp cx,0
 jne .g
 mov al,ah
.g:
 cmp cx,1
 jne .b
 mov bl,ah
.b:
 cmp cx,2
 jne .gr
 mov bh,ah
.gr:
 cmp cx,3
 jne .set
 mov al,ah
 mov bl,ah
 mov bh,ah
.set:
 call set_area_rgb
 pop bx
 mov ax,si
 shl ax,1
 shl ax,1
 add ax,GX0
 mov cx,ax
 add cx,4
 mov dx,bx
 add dx,10
 call fill
 pop cx
 inc si
 cmp si,64
 jb .step
 add bx,12
 inc cx
 cmp cx,4
 jb .ramp
 add bx,8
 ; ---- shapes ----
 mov si,g_shapes
 mov ax,GX0
 call g_title
 mov al,C_RED
 call set_lcol
 mov ax,GX0
 mov cx,GX0+50
 mov dx,bx
 add dx,36
 call frame
 mov al,C_GREEN
 call set_area
 mov ax,GX0+60
 mov cx,GX0+110
 call fill
 mov al,C_DBLUE
 call set_lcol
 mov ax,GX0+120
 mov cx,GX0+180
 call ellipse
 mov al,C_RED
 call set_lcol
 mov ax,GX0+190
 mov cx,GX0+226
 call ellipse
 ; nested frames
 mov si,0
.nest:
 mov ax,si
 add al,9
 call set_lcol
 mov ax,GX0+236
 add ax,si
 add ax,si
 push bx
 add bx,si
 add bx,si
 mov cx,GX0+286
 sub cx,si
 sub cx,si
 mov dx,bx
 add dx,36
 sub dx,si
 sub dx,si
 sub dx,si
 sub dx,si
 call frame
 pop bx
 inc si
 cmp si,5
 jb .nest
 add bx,46
 ; ---- area masks ----
 mov si,g_masks
 mov ax,GX0
 call g_title
 mov al,C_DBLUE
 call set_area
 xor si,si
.mask:
 mov al,[cs:mask_ids+si]
 push di
 KCALL 356                        ; GrSetAreaMask
 pop di
 mov ax,si
 mov cl,17
 mul cl
 add ax,GX0
 mov cx,ax
 add cx,14
 mov dx,bx
 add dx,14
 call fill
 inc si
 cmp si,MASKS
 jb .mask
 mov al,25                        ; SDM_100: solid again (as TETRIS does)
 push di
 KCALL 356
 pop di
 mov al,C_DGREY
 call set_lcol
 xor si,si
.mf:
 mov ax,si
 mov cl,17
 mul cl
 add ax,GX0
 mov cx,ax
 add cx,14
 mov dx,bx
 add dx,14
 call frame
 inc si
 cmp si,MASKS
 jb .mf
 ; ================= right column =================
 mov ax,[V_VW]
 cmp ax,GX1+260
 jge .right
 ret
.right:
 mov si,g_fonts
 mov ax,GX1
 mov bx,ROW_Y
 call g_title
 mov al,C_BLACK
 call set_tcol
 mov cx,F_ROMAN
 mov dx,14
 call set_font
 mov si,g_roman
 mov ax,GX1
 call text_cs
 add bx,20
 mov cx,F_SANS
 call set_font
 mov si,g_sans
 call text_cs
 add bx,20
 mov cx,F_MONO
 mov dx,12
 call set_font
 mov si,g_mono
 call text_cs
 add bx,18
 mov cx,F_BERK
 mov dx,10
 call set_font
 mov si,g_berk
 call text_cs
 add bx,18
 mov cx,F_UI
 mov dx,12
 call set_font
 mov al,C_RED
 call set_tcol
 mov ax,0x0020                    ; bold
 mov si,g_bold
 mov cx,GX1
 call style_word
 mov ax,0x0010                    ; italic
 mov si,g_ital
 mov cx,GX1+50
 call style_word
 mov ax,0x0001                    ; underline
 mov si,g_under
 mov cx,GX1+110
 call style_word
 add bx,18
 mov ax,0x0002                    ; strike through
 mov si,g_strike
 mov cx,GX1
 call style_word
 mov ax,0x0040                    ; outline
 mov si,g_outl
 mov cx,GX1+120
 call style_word
 add bx,18
 mov cx,F_ROMAN
 mov dx,24
 call set_font
 mov al,C_DBLUE
 call set_tcol
 mov si,g_big
 mov ax,GX1
 call text_cs
 add bx,36
 ; ---- bitmaps ----
 mov si,g_bmp
 mov ax,GX1
 call g_title
 mov ax,GX1
 call draw_icon_at
 ; 1 bit bitmap in the area colour
 mov al,C_DBLUE
 call set_area
 push ds
 push cs
 pop ds
 mov si,icon_mono
 mov ax,GX1+60
 xor dx,dx
 push bx
 push di
 KCALL 326                        ; GrDrawBitmap
 pop di
 pop bx
 pop ds
 cmp word [V_TC],0
 je .no24
 call draw_bmp24
.no24:
 add bx,34
 mov cx,F_UI
 mov dx,9
 call set_font
 mov al,C_DGREY
 call set_tcol
 mov si,g_bmp1
 mov ax,GX1
 call text_cs
 mov si,g_bmp3
 mov ax,GX1+70
 call text_cs
 mov si,g_bmp2
 mov ax,GX1+120
 call text_cs
 ret

MASKS equ 18
mask_ids: db 0,1,2,3,4,5,6,7,8,9, 25,33,41,49,57,65,73,81

; AX = style bits, CS:SI text, CX = x, BX = y
style_word:
 call set_style
 push ax
 mov ax,cx
 call text_cs
 pop ax
 xchg al,ah                       ; clear exactly these bits again
 call set_style
 ret

; 24 bit colour field 96x30 at GX1+120, row by row (type 03h, uncompacted)
draw_bmp24:
 push bx
 push di
 mov es,[V_LSEG]
 mov [es:L_TMPW3],di              ; gstate
 mov word [es:L_TMP],96
 mov word [es:L_TMP+2],1
 mov word [es:L_TMP+4],0x0300     ; compact 0, type 3
 xor dx,dx                        ; row
.row:
 mov es,[V_LSEG]
 mov di,L_TMP+6
 xor cx,cx
.px:
 mov al,cl                        ; red rises to the right
 shl al,1
 add al,60
 mov [es:di],al
 mov al,dl                        ; green rises downwards
 mov ah,8
 mul ah
 mov [es:di+1],al
 mov ah,255
 sub ah,al
 mov [es:di+2],ah                 ; blue falls
 add di,3
 inc cx
 cmp cx,96
 jb .px
 mov di,[es:L_TMPW3]
 pop ax                           ; (di)
 pop bx
 push bx
 push ax
 push dx
 mov ax,GX1+120
 add bx,dx
 push ds
 mov ds,[V_LSEG]
 mov si,L_TMP
 xor dx,dx
 KCALL 326                        ; GrDrawBitmap
 pop ds
 pop dx
 inc dx
 cmp dx,30
 jb .row
 pop di
 pop bx
 ret

; info page: the large icon on the right
draw_info_icon:
 mov ax,[V_VW]
 sub ax,90
 mov bx,ROW_Y+4
 call draw_icon_at
 ret

; ======================================================================
; page "Eingabe"
; ======================================================================
; after a key on the input page: redraw only the key list
keys_refresh:
 call begin_direct
 jc .r
 push bp
 mov al,C_WHITE
 call set_area
 mov ax,9
 mov bx,TOP_Y
 mov cx,GX1-4
 mov dx,[V_VH]
 sub dx,FOOT_H+5
 call fill
 call draw_keys
 pop bp
 call end_direct
.r:
 ret

record_key:
 mov es,[V_LSEG]
 push cx
 push dx
 mov si,H_KEYS+27                 ; move the 7 older entries down
 mov di,H_KEYS+31
 mov cx,28
 std
.m:
 mov al,[es:si]
 mov [es:di],al
 dec si
 dec di
 loop .m
 cld
 pop dx
 pop cx
 mov [es:H_KEYS],cx
 mov [es:H_KEYS+2],dx
 inc word [es:H_NKEY]
 ret

draw_input:
 push bp
 call draw_input2
 pop bp
 ret
draw_input2:
 call draw_keys
 jmp input_mouse

; keyboard part of the input page (left column)
draw_keys:
 mov si,i_keys
 mov ax,GX0
 mov bx,ROW_Y
 call g_title
 mov es,[V_LSEG]
 cmp word [es:H_NKEY],0
 jne .k
 mov cx,F_UI
 mov dx,10
 call set_font
 mov al,C_DGREY
 call set_tcol
 mov si,i_nokey
 call text_cs
 jmp .kend
.k:
 mov cx,F_UI
 mov dx,10
 call set_font
 mov al,C_DGREY
 call set_tcol
 mov si,i_last
 call text_cs
 add bx,16
 mov cx,F_MONO
 mov dx,10
 call set_font
 mov si,i_hdr
 call text_cs
 add bx,15
 mov al,C_BLACK
 call set_tcol
 xor si,si                        ; entry
.ent:
 mov es,[V_LSEG]
 cmp si,[es:H_NKEY]
 jae .kend
 push si
 call v_reset
 mov ax,[es:H_NKEY]
 sub ax,si
 call v_n
 mov al,'.'
 call v_c
 call v_sp
 call v_sp
 mov bp,si
 shl bp,1
 shl bp,1
 mov cx,[es:H_KEYS+bp]
 mov dx,[es:H_KEYS+bp+2]
 or ch,ch
 jnz .nochr
 cmp cl,' '
 jb .nochr
 mov al,0x27
 call v_c
 mov al,cl
 call v_c
 mov al,0x27
 call v_c
 jmp .hx
.nochr:
 mov al,'-'
 call v_c
 call v_sp
 call v_sp
.hx:
 mov al,' '
 call v_c
 call v_sp
 call v_sp
 call v_sp
 call v_sp
 mov ax,cx
 push cx
 mov cl,4
 call v_hexh
 pop cx
 call v_sp
 call v_sp
 mov al,dl
 xor ah,ah
 push cx
 mov cl,2
 call v_hex
 mov al,'/'
 call v_c
 mov al,dh
 xor ah,ah
 call v_hex
 pop cx
 pop si
 push si
 mov si,L_VAL
 mov ax,GX0
 call text_ls
 pop si
 add bx,15
 inc si
 cmp si,8
 jb .ent
.kend:
 ret

input_mouse:
 mov si,i_mouse
 mov ax,GX1
 mov bx,ROW_Y
 call g_title
 ; drawing pad
 mov es,[V_LSEG]
 mov word [es:L_PADX],GX1
 mov word [es:L_PADY],ROW_Y+146
 mov ax,[V_VW]
 sub ax,GX1+24
 cmp ax,80
 jge .pw
 mov ax,80
.pw:
 mov [es:L_PADW],ax
 mov ax,[V_VH]
 sub ax,ROW_Y+146+FOOT_H+30
 cmp ax,40
 jge .ph
 mov ax,40
.ph:
 mov [es:L_PADH],ax
 mov al,C_DGREY
 call set_lcol
 mov ax,[es:L_PADX]
 mov bx,[es:L_PADY]
 mov cx,ax
 add cx,[es:L_PADW]
 mov dx,bx
 add dx,[es:L_PADH]
 call frame
 mov cx,F_UI
 mov dx,9
 call set_font
 mov al,C_DGREY
 call set_tcol
 add bx,[es:L_PADH]
 add bx,4
 mov ax,GX1
 mov si,i_pad
 call text_cs
 call mouse_values
 ret

; called with the paint gstate in DI: mouse position, clicks
mouse_values:
 mov al,C_WHITE
 call set_area
 mov ax,GX1
 mov bx,ROW_Y+18
 mov cx,[V_VW]
 sub cx,20
 mov dx,ROW_Y+142
 call fill
 mov cx,F_UI
 mov dx,10
 call set_font
 mov al,C_DGREY
 call set_tcol
 mov si,i_pos
 mov ax,GX1
 mov bx,ROW_Y+20
 call text_cs
 mov si,i_clicks
 add bx,20
 call text_cs
 mov si,i_lastc
 add bx,20
 call text_cs
 mov si,i_bp
 add bx,20
 call text_cs
 mov si,i_ends
 add bx,20
 call text_cs
 mov si,i_drags
 add bx,20
 call text_cs
 mov ax,0x0020
 call set_style
 mov al,C_BLACK
 call set_tcol
 call v_reset
 mov ax,[V_MX]
 call v_n
 mov al,','
 call v_c
 call v_sp
 mov ax,[V_MY]
 call v_n
 mov si,L_VAL
 mov ax,GX1+110
 mov bx,ROW_Y+20
 call text_ls
 call v_reset
 mov es,[V_LSEG]
 mov ax,[es:H_CLICKS]
 call v_n
 mov ax,GX1+110
 add bx,20
 call text_ls
 call v_reset
 mov es,[V_LSEG]
 mov ax,[es:H_CX]
 call v_n
 mov al,','
 call v_c
 call v_sp
 mov ax,[es:H_CY]
 call v_n
 mov ax,GX1+110
 add bx,20
 call text_ls
 call v_reset
 mov es,[V_LSEG]
 mov ax,[es:H_BP]
 mov cl,4
 call v_hexh
 cmp word [V_DOWN],0
 je .up
 push si
 call v_sp
 call v_sp
 mov si,i_held
 call v_s
 pop si
.up:
 mov ax,GX1+110
 add bx,20
 call text_ls
 call v_reset
 mov es,[V_LSEG]
 mov ax,[es:H_ENDS]
 call v_n
 mov ax,GX1+130
 add bx,20
 call text_ls
 call v_reset
 mov es,[V_LSEG]
 mov ax,[es:H_DRAGS]
 call v_n
 mov ax,GX1+130
 add bx,20
 call text_ls
 mov ax,0xff00
 call set_style
 ret

; update only the mouse values (outside an exposed event)
input_status:
 call begin_direct
 jc .r
 call mouse_values
 call end_direct
.r:
 ret

; stroke in the pad: from the last point to V_MX, V_MY in steps of
; 2 pixels (3x3 dots), so fast movements still give a line
pad_dot:
 call begin_direct
 jc .r
 mov al,C_DBLUE
 call set_area
 mov es,[V_LSEG]
 mov ax,[V_MX]
 mov bx,[V_MY]
 cmp word [es:L_HAVE],0
 jne .line
 call pad_plot
 jmp .done
.line:
 ; steps = max(|dx|,|dy|) / 2 + 1
 mov cx,ax
 sub cx,[es:L_LX]
 mov [es:L_TMPW],cx               ; dx
 jge .a1
 neg cx
.a1:
 mov dx,bx
 sub dx,[es:L_LY]
 mov [es:L_TMPW2],dx              ; dy
 jge .a2
 neg dx
.a2:
 cmp cx,dx
 jge .a3
 mov cx,dx
.a3:
 shr cx,1
 inc cx
 cmp cx,400
 jbe .a4
 mov cx,400
.a4:
 mov [es:L_TMPW3],cx              ; steps
 mov si,1
.step:
 mov ax,[es:L_TMPW]
 imul si
 idiv word [es:L_TMPW3]
 add ax,[es:L_LX]
 push ax
 mov ax,[es:L_TMPW2]
 imul si
 idiv word [es:L_TMPW3]
 add ax,[es:L_LY]
 mov bx,ax
 pop ax
 push si
 call pad_plot
 pop si
 mov es,[V_LSEG]
 inc si
 cmp si,[es:L_TMPW3]
 jbe .step
.done:
 mov es,[V_LSEG]
 mov ax,[V_MX]
 mov [es:L_LX],ax
 mov ax,[V_MY]
 mov [es:L_LY],ax
 mov word [es:L_HAVE],1
 call end_direct
.r:
 ret

; 3x3 dot at AX,BX if inside the pad (DI = gstate)
pad_plot:
 mov es,[V_LSEG]
 mov cx,[es:L_PADX]
 cmp ax,cx
 jle .r
 add cx,[es:L_PADW]
 sub cx,3
 cmp ax,cx
 jge .r
 mov cx,[es:L_PADY]
 cmp bx,cx
 jle .r
 add cx,[es:L_PADH]
 sub cx,3
 cmp bx,cx
 jge .r
 mov cx,ax
 add cx,3
 mov dx,bx
 add dx,3
 call fill
.r:
 ret
