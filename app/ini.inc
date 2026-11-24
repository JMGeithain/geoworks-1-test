; SPDX-License-Identifier: MIT
; ini.inc - read GEOS.INI (GEOWORKS directory) into the state block and
; look up "[section]" / "key = value" (case-insensitive, ";" = comment,
; values in { } over several lines are joined with ", ").

s_ini: db 'GEOS.INI',0

load_ini:
 mov es,[V_LSEG]
 mov byte [es:L_INI],0
 mov word [es:H_INIOK],0
 KCALL 73                         ; FilePushDir
 mov ax,0
 KCALL 76                         ; FileSetStandardPath SP_TOP
 jc .pop
 push ds
 push cs
 pop ds
 mov dx,s_ini
 mov al,0x40                      ; deny none, read
 KCALL 52                         ; FileOpen
 pop ds
 jc .pop
 mov bx,ax
 push bx
 push ds
 mov ds,[V_LSEG]
 mov dx,L_INI
 mov cx,INI_MAX-1
 mov al,0
 KCALL 58                         ; FileRead
 jnc .full
 cmp cx,INI_MAX-1                 ; short read: CX = bytes read
 jbe .set
 xor cx,cx
 jmp .set
.full:
 mov cx,INI_MAX-1
.set:
 mov si,L_INI
 add si,cx
 mov byte [si],0
 pop ds
 pop bx
 mov al,0
 KCALL 54                         ; FileClose
 mov es,[V_LSEG]
 mov word [es:H_INIOK],1
.pop:
 KCALL 74                         ; FilePopDir
 ret

; AL -> lower case
lower:
 cmp al,'A'
 jb .r
 cmp al,'Z'
 ja .r
 add al,0x20
.r:
 ret

; ES:DI -> first character that is no space/tab
ini_skipws:
 cmp byte [es:di],' '
 je .s
 cmp byte [es:di],9
 jne .r
.s:
 inc di
 jmp ini_skipws
.r:
 ret

; CS:SI = section (without brackets), CS:BX = key.
; The value is appended to L_VAL. CF = not found.
ini_get:
 push ax
 push bx
 push cx
 push dx
 push si
 push di
 push es
 mov es,[V_LSEG]
 cmp word [es:H_INIOK],0
 jne .go
 jmp .nf
.go:
 mov di,L_INI
 xor dx,dx                        ; 1 = inside the section
.line:
 call ini_skipws
 mov al,[es:di]
 or al,al
 jnz .l1
 jmp .nf
.l1:
 cmp al,'['
 jne .notsec
 inc di
 xor dx,dx
 push si
.sc:
 mov ah,[cs:si]
 or ah,ah
 jz .send
 mov al,[es:di]
 call lower
 xchg al,ah
 call lower
 cmp al,ah
 jne .sno
 inc si
 inc di
 jmp .sc
.send:
 cmp byte [es:di],']'
 jne .sno
 mov dx,1
.sno:
 pop si
 jmp .eol
.notsec:
 or dx,dx
 jz .eol
 cmp al,';'
 je .eol
 push bx
.kc:
 mov ah,[cs:bx]
 or ah,ah
 jz .kend
 mov al,[es:di]
 call lower
 xchg al,ah
 call lower
 cmp al,ah
 jne .kno
 inc bx
 inc di
 jmp .kc
.kend:
 pop bx
 call ini_skipws
 cmp byte [es:di],'='
 jne .eol
 inc di
 call ini_skipws
 cmp byte [es:di],'{'
 je .block
 call ini_copyline
 jmp .found
.kno:
 pop bx
.eol:
 mov al,[es:di]
 or al,al
 jz .nf
 inc di
 cmp al,10
 jne .eol
 jmp .line
.block:
 xor cx,cx                        ; entries so far
.bnext:                           ; to the next line
 mov al,[es:di]
 or al,al
 jz .found
 inc di
 cmp al,10
 jne .bnext
 call ini_skipws
 mov al,[es:di]
 or al,al
 jz .found
 cmp al,'}'
 je .found
 cmp al,13
 je .bnext
 cmp al,10
 je .bnext
 jcxz .first
 mov al,','
 call v_c
 mov al,' '
 call v_c
.first:
 inc cx
 call ini_copyline
 jmp .bnext
.found:
 pop es
 pop di
 pop si
 pop dx
 pop cx
 pop bx
 pop ax
 clc
 ret
.nf:
 pop es
 pop di
 pop si
 pop dx
 pop cx
 pop bx
 pop ax
 stc
 ret

; ES:DI = value -> L_VAL up to the line end, trailing blanks removed
ini_copyline:
 mov al,[es:di]
 or al,al
 jz .trim
 cmp al,13
 je .trim
 cmp al,10
 je .trim
 cmp al,9
 jne .c
 mov al,' '
.c:
 call v_c
 inc di
 jmp ini_copyline
.trim:
 push di
 mov di,[es:L_VP]
.t:
 cmp di,L_VAL
 je .te
 cmp byte [es:di-1],' '
 jne .te
 dec di
 mov byte [es:di],0
 jmp .t
.te:
 mov [es:L_VP],di
 pop di
 ret

; as ini_get, but "-" when the entry is missing
ini_get_or_dash:
 call ini_get
 jnc .r
 push si
 mov si,s_dash
 call v_s
 pop si
.r:
 ret
