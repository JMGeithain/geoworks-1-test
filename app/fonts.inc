; SPDX-License-Identifier: MIT
; fonts.inc - page "9 Schriften": the font files in GEOWORKS\FONT.
;
; GeoWorks 1 has no GrEnumFonts for applications, so the page reads the
; files themselves (FileEnum, kernel ordinal 65, as in GeoMaler) and their
; header. Header of a .FNT file (checked with the files of GeoWorks 1.2):
;   +0  "BSWF"            +4 word 0100h      +6 word size of the font info
;   +8  word font ID      +0Ah word maker (0 bitmap, 1000h Nimbus, F000h printer)
;   +0Ch byte family      +0Dh 20 bytes face name
;   +21h word start, +23h word end of the bitmap size table   (offsets + 6)
;   +25h word start, +27h word end of the outline table       (offsets + 6)
; bitmap size entry (10 bytes): style, size (fraction byte, integer word),
; data size word, file position dword. Style 0 = plain.

%define ENT_SIZE  22        ; FileEnum entry: attributes +0, name +9
%define ENT_NAME  9
%define FE_MAX    150
%define FONT_MAX  150
%define FR_SIZE   48        ; font record
%define FR_NAME   0         ; 21 bytes face name
%define FR_FILE   21        ; 13 bytes DOS file name
%define FR_ID     34
%define FR_MAKER  36
%define FR_OUTL   38        ; 1 = outline font
%define FR_NSZ    39        ; number of bitmap sizes
%define FR_SIZES  40        ; up to 8 sizes (bytes, ascending)
%define FL_Y      (ROW_Y+40)
%define FROW_H    22
%define FX_NAME   22
%define FX_FILE   168
%define FX_ID     252
%define FX_KIND   338
%define FX_SAMPLE 462

s_fontdir: db 'FONT',0
s_fcount:  db 'Installierte Schriften: ',0
s_fsys:    db 'Systemschrift: ',0
s_fout:    db ' ',0xe1,' Font-Treiber: ',0
s_paren:   db ' (',0
s_hparen:  db 'h)',0

; read all font headers (at the start, in the process thread)
font_scan:
 mov es,[V_LSEG]
 mov word [es:L_NFONT],0
 KCALL 73                         ; FilePushDir
 mov ax,0
 KCALL 76                         ; FileSetStandardPath SP_TOP
 jc .pop
 push ds
 push cs
 pop ds
 xor bx,bx
 mov dx,s_fontdir
 KCALL 51                         ; FileSetCurrentPath FONT
 pop ds
 jc .pop
 push ds
 mov ds,[V_LSEG]
 mov si,L_FPATH
 mov byte [si],0
 mov cx,120
 KCALL 75                         ; FileGetCurrentPath -> BX disk
 pop ds
 jc .pop
 mov es,[V_LSEG]
 mov [es:L_FDISK],bx
 call fe_call
 jc .pop
 cmp bx,FE_MAX
 jbe .n
 mov bx,FE_MAX
.n:
 xor si,si
.e:
 cmp si,bx
 jae .pop
 push si
 push bx
 mov ax,si
 mov cx,ENT_SIZE
 mul cx
 mov di,ax
 add di,L_FENT
 mov es,[V_LSEG]
 test byte [es:di],0x10           ; folder
 jnz .skip
 lea bx,[di+ENT_NAME]
 cmp byte [es:bx],0
 jne .nm
 inc bx
.nm:
 call is_fnt
 jc .skip
 call font_read
.skip:
 pop bx
 pop si
 inc si
 jmp .e
.pop:
 KCALL 74                         ; FilePopDir
 call font_sort
 ret

; FileEnum of L_FPATH into L_FENT. CF = error, BX = entries
fe_call:
 push bp
 sub sp,0x30
 mov bp,sp
 push ss
 pop es
 mov di,sp
 cld
 xor ax,ax
 mov cx,0x18
 rep stosw
 mov byte [bp+0],0xfc             ; all files
 mov word [bp+1],0x4000
 mov byte [bp+3],0x20
 mov word [bp+4],L_FPATH
 mov ax,[V_LSEG]
 mov [bp+6],ax
 mov es,ax
 mov ax,[es:L_FDISK]
 mov [bp+8],ax
 mov word [bp+0x12],L_FENT
 mov ax,[V_LSEG]
 mov [bp+0x14],ax
 mov word [bp+0x16],FE_MAX
 KCALL 65                         ; FileEnum (removes the block)
 pop bp
 ret

; ES:BX = file name -> CF = not "*.FNT"
is_fnt:
 push ax
 push si
 mov si,bx
.l:
 cmp byte [es:si],0
 je .e
 inc si
 jmp .l
.e:
 mov ax,si
 sub ax,bx
 cmp ax,5
 jb .no
 cmp byte [es:si-4],'.'
 jne .no
 mov al,[es:si-3]
 and al,0xdf
 cmp al,'F'
 jne .no
 mov al,[es:si-2]
 and al,0xdf
 cmp al,'N'
 jne .no
 mov al,[es:si-1]
 and al,0xdf
 cmp al,'T'
 jne .no
 pop si
 pop ax
 clc
 ret
.no:
 pop si
 pop ax
 stc
 ret

; ES:BX = file name (in the state block, current folder FONT): read the
; header and add a font record
font_read:
 mov es,[V_LSEG]
 cmp word [es:L_NFONT],FONT_MAX
 jb .room
 ret
.room:
 mov [es:L_FNAME],bx
 mov di,L_TMP
 mov cx,128
 xor ax,ax
 cld
 rep stosw
 push ds
 mov ds,[V_LSEG]
 mov dx,bx
 mov al,0x40                      ; read, deny none
 KCALL 52                         ; FileOpen
 pop ds
 jnc .open
 ret
.open:
 mov bx,ax
 push bx
 push ds
 mov ds,[V_LSEG]
 mov dx,L_TMP
 mov cx,256
 mov al,0
 KCALL 58                         ; FileRead (a short read sets CF)
 pop ds
 pop bx
 mov al,0
 KCALL 54                         ; FileClose
 mov es,[V_LSEG]
 cmp word [es:L_TMP],'BS'
 jne .r
 cmp word [es:L_TMP+2],'WF'
 jne .r
 ; the record
 mov ax,[es:L_NFONT]
 mov cx,FR_SIZE
 mul cx
 add ax,L_FONTS
 mov bx,ax
 push ds
 mov ds,[V_LSEG]
 push ds
 pop es
 cld
 mov di,bx
 mov cx,FR_SIZE/2
 xor ax,ax
 rep stosw
 mov si,L_TMP+0x0d                ; face name
 lea di,[bx+FR_NAME]
 mov cx,20
.cn:
 lodsb
 or al,al
 jz .cne
 stosb
 loop .cn
.cne:
 mov si,[L_FNAME]                 ; file name
 lea di,[bx+FR_FILE]
 mov cx,12
.cf:
 lodsb
 or al,al
 jz .cfe
 stosb
 loop .cf
.cfe:
 cmp byte [bx+FR_NAME],0          ; no face name: the file name
 jne .hasn
 lea si,[bx+FR_FILE]
 lea di,[bx+FR_NAME]
 mov cx,13
 rep movsb
.hasn:
 mov ax,[L_TMP+8]
 mov [bx+FR_ID],ax
 mov ax,[L_TMP+0x0a]
 mov [bx+FR_MAKER],ax
 mov ax,[L_TMP+0x27]
 cmp ax,[L_TMP+0x25]
 jbe .bm
 mov byte [bx+FR_OUTL],1
.bm:
 mov si,[L_TMP+0x21]
 mov dx,[L_TMP+0x23]
 add si,6
 add dx,6
 cmp dx,256
 jbe .sz
 mov dx,256
.sz:
 mov ax,si
 add ax,10
 cmp ax,dx
 ja .szd
 cmp byte [L_TMP+si],0            ; plain style only
 jne .nx
 cmp byte [L_TMP+si+3],0
 jne .nx
 mov al,[L_TMP+si+2]
 or al,al
 jz .nx
 call add_size
.nx:
 add si,10
 jmp .sz
.szd:
 inc word [L_NFONT]
 pop ds
.r:
 ret

; ES:BX = record, AL = size: insert ascending, no duplicates, at most 8
add_size:
 push cx
 push dx
 push si
 push di
 mov cl,[es:bx+FR_NSZ]
 xor ch,ch
 xor si,si
.f:
 cmp si,cx
 jae .ins
 mov dl,[es:bx+si+FR_SIZES]
 cmp dl,al
 je .r
 ja .ins
 inc si
 jmp .f
.ins:
 cmp cx,8
 jae .r
 mov di,cx
.sh:
 cmp di,si
 jbe .put
 mov dl,[es:bx+di+FR_SIZES-1]
 mov [es:bx+di+FR_SIZES],dl
 dec di
 jmp .sh
.put:
 mov [es:bx+si+FR_SIZES],al
 inc byte [es:bx+FR_NSZ]
.r:
 pop di
 pop si
 pop dx
 pop cx
 ret

; sort the records by name (A = a)
font_sort:
 mov es,[V_LSEG]
 cmp word [es:L_NFONT],2
 jae .go
 ret
.go:
 push ds
 mov ds,[V_LSEG]
.pass:
 xor dx,dx
 mov bx,L_FONTS
 mov cx,[L_NFONT]
 dec cx
.p:
 push cx
 mov si,bx
 lea di,[bx+FR_SIZE]
 call name_less                   ; CF = second before first
 jnc .ns
 mov si,bx
 lea di,[bx+FR_SIZE]
 mov cx,FR_SIZE/2
.sw:
 mov ax,[si]
 xchg ax,[di]
 mov [si],ax
 add si,2
 add di,2
 loop .sw
 mov dx,1
.ns:
 add bx,FR_SIZE
 pop cx
 loop .p
 or dx,dx
 jnz .pass
 pop ds
 ret

; DS:SI, DS:DI names -> CF = DI before SI
name_less:
 push ax
 push si
 push di
.l:
 mov al,[di]
 call lower
 mov ah,al
 mov al,[si]
 call lower
 cmp ah,al
 jb .yes
 ja .no
 or al,al
 jz .no
 inc si
 inc di
 jmp .l
.yes:
 pop di
 pop si
 pop ax
 stc
 ret
.no:
 pop di
 pop si
 pop ax
 clc
 ret

; ----------------------------------------------------------------------
; the page
; ----------------------------------------------------------------------
draw_fonts:
 mov cx,F_UI
 mov dx,12
 call set_font
 mov ax,0x0020
 call set_style
 mov al,C_DBLUE
 call set_tcol
 call v_reset
 VS s_fcount
 mov es,[V_LSEG]
 mov ax,[es:L_NFONT]
 call v_n
 mov si,L_VAL
 mov ax,18
 mov bx,ROW_Y
 call text_ls
 ; system font from GEOS.INI
 mov cx,F_UI
 mov dx,10
 call set_font
 mov ax,0xff00
 call set_style
 mov al,C_DGREY
 call set_tcol
 call v_reset
 VS s_fsys
 mov si,s_ssystem
 mov bx,s_kfontid
 call ini_get_or_dash
 call v_sp
 mov si,s_ssystem
 mov bx,s_kfontsize
 call ini_get_or_dash
 VS s_fout
 mov si,s_ssystem
 mov bx,s_kfontdrv
 call ini_get_or_dash
 mov si,L_VAL
 mov ax,250
 mov bx,ROW_Y+3
 call text_ls
 ; column titles and line
 mov cx,F_UI
 mov dx,9
 call set_font
 mov bx,ROW_Y+21
 mov si,c_fname
 mov ax,FX_NAME
 call text_cs
 mov si,c_ffile
 mov ax,FX_FILE
 call text_cs
 mov si,c_fid
 mov ax,FX_ID
 call text_cs
 mov si,c_fkind
 mov ax,FX_KIND
 call text_cs
 cmp word [V_VW],600
 jb .nos
 mov si,c_fsample
 mov ax,FX_SAMPLE
 call text_cs
.nos:
 mov al,C_DGREY
 call set_area
 mov ax,18
 mov bx,ROW_Y+34
 mov cx,[V_VW]
 sub cx,18
 mov dx,ROW_Y+35
 call fill
 mov es,[V_LSEG]
 cmp word [es:L_NFONT],0
 jne .have
 mov word [V_NROW],0
 mov cx,F_UI
 mov dx,10
 call set_font
 mov si,s_nofonts
 mov ax,24
 mov bx,FL_Y+3
 call text_cs
 ret
.have:
 ; visible rows and scroll limit
 mov ax,[V_VH]
 sub ax,FOOT_H+6+FL_Y
 jg .h1
 mov ax,FROW_H
.h1:
 xor dx,dx
 mov cx,FROW_H
 div cx
 or ax,ax
 jnz .h2
 inc ax
.h2:
 mov cx,ax                        ; visible rows
 mov es,[V_LSEG]
 mov ax,[es:L_NFONT]
 sub ax,cx
 jg .h3
 xor ax,ax
.h3:
 inc ax
 mov [V_NROW],ax
 cmp [V_SCR],ax
 jb .h4
 dec ax
 mov [V_SCR],ax
.h4:
 mov si,[V_SCR]                   ; font index
 mov bx,FL_Y
 xor dx,dx                        ; row
.row:
 mov es,[V_LSEG]
 cmp si,[es:L_NFONT]
 jae .end
 cmp dx,cx
 jae .end
 push bx
 push cx
 push dx
 push si
 call draw_font_row
 pop si
 pop dx
 pop cx
 pop bx
 inc si
 inc dx
 add bx,FROW_H
 jmp .row
.end:
 ; samples that run over the right edge: border again
 cmp word [V_VW],600
 jb .r
 mov al,C_GREY
 call set_area
 mov ax,[V_VW]
 sub ax,7
 mov bx,TOP_Y
 mov cx,[V_VW]
 mov dx,[V_VH]
 sub dx,FOOT_H
 call fill
 mov al,C_DGREY
 call set_lcol
 mov ax,8
 mov bx,TOP_Y-1
 mov cx,[V_VW]
 sub cx,8
 mov dx,[V_VH]
 sub dx,FOOT_H+4
 call frame
.r:
 mov ax,0xff00
 call set_style
 ret

; SI = font index, DX = row on the page, BX = y
draw_font_row:
 test dl,1
 jz .nb
 mov al,C_GREY
 call set_area
 mov al,73                        ; SDM_25
 call set_mask
 mov ax,14
 push bx
 sub bx,2
 mov cx,[V_VW]
 sub cx,14
 mov dx,bx
 add dx,FROW_H
 call fill
 pop bx
 mov al,25                        ; SDM_100
 call set_mask
.nb:
 mov ax,si
 mov cx,FR_SIZE
 mul cx
 add ax,L_FONTS
 mov si,ax                        ; record in the state block
 mov cx,F_UI
 mov dx,10
 call set_font
 mov ax,0x0020
 call set_style
 mov al,C_BLACK
 call set_tcol
 push si
 add si,FR_NAME
 mov ax,FX_NAME
 add bx,3
 call text_ls
 pop si
 mov ax,0xff00
 call set_style
 push si
 add si,FR_FILE
 mov ax,FX_FILE
 call text_ls
 pop si
 ; "514 (0202h)"
 mov es,[V_LSEG]
 call v_reset
 mov ax,[es:si+FR_ID]
 call v_n
 VS s_paren
 mov cl,4
 call v_hex
 VS s_hparen
 push si
 mov si,L_VAL
 mov ax,FX_ID
 call text_ls
 pop si
 ; kind and sizes
 mov al,C_DGREY
 call set_tcol
 call v_reset
 mov es,[V_LSEG]
 mov ax,[es:si+FR_MAKER]
 and ax,0xf000
 cmp ax,0xf000
 jne .k1
 VS s_kprinter
 jmp .k3
.k1:
 cmp byte [es:si+FR_OUTL],0
 je .k2
 VS s_koutline
 jmp .k3
.k2:
 VS s_kbitmap
.k3:
 cmp byte [es:si+FR_OUTL],0
 jne .k5
 mov cl,[es:si+FR_NSZ]
 xor ch,ch
 jcxz .k5
 push bx
 push si
 mov bx,si
 xor si,si
.k4:
 mov al,'/'
 or si,si
 jnz .k4s
 mov al,' '
.k4s:
 call v_c
 mov al,[es:bx+si+FR_SIZES]
 xor ah,ah
 call v_n
 inc si
 loop .k4
 pop si
 pop bx
.k5:
 push si
 mov si,L_VAL
 mov ax,FX_KIND
 call text_ls
 pop si
 ; sample in the font itself
 cmp word [V_VW],600
 jb .r
 mov es,[V_LSEG]
 call pick_size
 mov cx,[es:si+FR_ID]
 call set_font
 mov al,C_DBLUE
 call set_tcol
 mov si,t_fsample
 mov ax,FX_SAMPLE
 sub bx,2
 call text_cs
.r:
 ret

; ES:SI = record -> DX = size for the sample: 14 point, or the bitmap
; size nearest to it
pick_size:
 mov dx,14
 cmp byte [es:si+FR_OUTL],0
 jne .r
 cmp byte [es:si+FR_NSZ],0
 je .r
 push ax
 push bx
 push cx
 push bp
 mov bp,0xffff
 xor bx,bx
.l:
 mov al,[es:si+bx+FR_SIZES]
 xor ah,ah
 mov cx,ax
 sub cx,14
 jns .p
 neg cx
.p:
 cmp cx,bp
 jae .n
 mov bp,cx
 mov dx,ax
.n:
 inc bx
 cmp bl,[es:si+FR_NSZ]
 jb .l
 pop bp
 pop cx
 pop bx
 pop ax
.r:
 ret

set_mask:                         ; AL = system draw mask
 push ax
 push di
 KCALL 356                        ; GrSetAreaMask
 pop di
 pop ax
 ret
