; SPDX-License-Identifier: MIT
; output.inc - the report as text (file DOCUMENT\GWTEST.TXT, clipboard)
; and on paper (PrintControl of the spool library, as in the PDF-Betrachter).

%define L_TXM   1060              ; text mode: 0 GEOS/clipboard, 1 ASCII file
%define L_TXP   1062              ; write pointer
%define L_TXC   1064              ; column
%define P_DRAW  1066              ; print walk: 0 count, 1 draw
%define P_PRW   1068              ; printable width
%define P_LH    13                ; line height in points
%define P_HDR   30                ; header height
%define P_MAXP  20                ; at most 20 sheets
%define TXT_COL 34                ; value column in the text file

; ----------------------------------------------------------------------
; text building
; ----------------------------------------------------------------------
t_c:                              ; AL
 push ax
 push bx
 push di
 push es
 mov es,[V_LSEG]
 cmp byte [es:L_TXM],0
 je .one
 cmp al,0x80
 jb .one
 mov bx,translit
.t:
 cmp byte [cs:bx],0
 je .q
 cmp al,[cs:bx]
 je .hit
 inc bx
.s:
 inc bx
 cmp byte [cs:bx-1],0
 jne .s
 jmp .t
.hit:
 inc bx
.h:
 mov al,[cs:bx]
 or al,al
 jz .e
 call .put
 inc bx
 jmp .h
.q:
 mov al,'?'
.one:
 call .put
.e:
 pop es
 pop di
 pop bx
 pop ax
 ret
.put:
 mov di,[es:L_TXP]
 cmp di,L_TXT+TXT_MAX-8
 jae .full
 mov [es:di],al
 inc di
 mov [es:L_TXP],di
 inc word [es:L_TXC]
.full:
 ret

translit:
 db 0x8a,'ae',0, 0x9a,'oe',0, 0x9f,'ue',0, 0x80,'Ae',0, 0x85,'Oe',0
 db 0x86,'Ue',0, 0xa7,'ss',0, 0xd0,'-',0, 0xd1,'-',0, 0xc9,'...',0
 db 0xe1,'-',0, 0
t_s:                              ; CS:SI
 push ax
 push si
.l:
 mov al,[cs:si]
 or al,al
 jz .e
 call t_c
 inc si
 jmp .l
.e:
 pop si
 pop ax
 ret
t_ls:                             ; state block:SI
 push ax
 push si
 push es
 mov es,[V_LSEG]
.l:
 mov al,[es:si]
 or al,al
 jz .e
 call t_c
 inc si
 jmp .l
.e:
 pop es
 pop si
 pop ax
 ret
t_nl:
 push ax
 push es
 mov al,13
 call t_c
 mov es,[V_LSEG]
 cmp byte [es:L_TXM],0
 je .r
 mov al,10
 call t_c
.r:
 mov word [es:L_TXC],0
 pop es
 pop ax
 ret

; AL = mode -> report text at L_TXT, CX = length
build_text:
 mov es,[V_LSEG]
 mov [es:L_TXM],al
 mov word [es:L_TXP],L_TXT
 mov word [es:L_TXC],0
 call build_report
 mov si,s_rtitle
 call t_s
 call t_nl
 mov si,s_rdate
 call t_s
 mov al,' '
 call t_c
 call v_reset
 call v_datetime
 mov si,L_VAL
 call t_ls
 call t_nl
 xor cx,cx                        ; page
.page:
 call t_nl
 mov si,s_eq
 call t_s
 mov bx,cx
 shl bx,1
 mov si,[cs:tab_names+bx]
 call t_s
 mov si,s_eq2
 call t_s
 call t_nl
 mov es,[V_LSEG]
 mov si,L_REP
.rec:
 mov al,[es:si]
 cmp al,0xff
 je .pend
 cmp al,cl
 je .mine
 call skip_rec
 jmp .rec
.mine:
 cmp byte [es:si+1],1
 jne .row
 add si,2
 call t_nl
 call t_ls
 call t_nl
 call skip_str
 call skip_str
 jmp .rec
.row:
 add si,2
 mov al,' '
 call t_c
 call t_c
 call t_ls
 call skip_str
 cmp byte [es:L_TXM],0
 jne .pad
 mov al,9                         ; clipboard: tab before the value
 call t_c
 jmp .val
.pad:
 mov al,' '
.p:
 call t_c
 cmp word [es:L_TXC],TXT_COL
 jb .p
.val:
 call t_ls
 call skip_str
 call t_nl
 jmp .rec
.pend:
 inc cx
 cmp cx,6
 jb .page
 mov es,[V_LSEG]
 mov cx,[es:L_TXP]
 sub cx,L_TXT
 ret
s_eq: db '=== ',0
s_eq2: db ' ===',0

; ----------------------------------------------------------------------
; Datei > Bericht speichern: DOCUMENT\GWTEST.TXT
; ----------------------------------------------------------------------
do_save:
 cmp word [V_INIT],0
 jne .go
 ret
.go:
 mov al,1
 call build_text
 mov es,[V_LSEG]
 mov [es:L_CNT],cx
 KCALL 73                         ; FilePushDir
 mov ax,0
 KCALL 76                         ; FileSetStandardPath SP_TOP
 jc .fail
 push ds
 push cs
 pop ds
 mov bx,0
 mov dx,s_docdir
 KCALL 51                         ; FileSetCurrentPath DOCUMENT
 pop ds
 jc .fail
 push ds
 push cs
 pop ds
 mov dx,s_txtname
 mov ax,0x00c1                    ; create/truncate, deny none, write
 xor cx,cx
 KCALL 53                         ; FileCreate
 pop ds
 jc .fail
 mov bx,ax
 push bx
 mov es,[V_LSEG]
 mov cx,[es:L_CNT]
 push ds
 mov ds,[V_LSEG]
 mov dx,L_TXT
 mov al,0
 KCALL 59                         ; FileWrite
 pop ds
 sbb si,si                        ; -1 = failed
 pop bx
 push si
 mov al,0
 KCALL 54                         ; FileClose
 pop si
 or si,si
 jnz .fail
 KCALL 74                         ; FilePopDir
 mov ax,m_saved
 call show_msg
 ret
.fail:
 KCALL 74
 mov ax,m_nsave
 call show_msg
 ret

; ----------------------------------------------------------------------
; Datei > In Zwischenablage kopieren
; ----------------------------------------------------------------------
do_copy:
 cmp word [V_INIT],0
 jne .go
 ret
.go:
 mov al,0
 call build_text
 push ds
 mov ds,[V_LSEG]
 mov si,L_TXT
 mov dx,s_clipname
 call g1_clip_text
 pop ds
 mov ax,m_copied
 jnc .ok
 mov ax,m_ncopy
.ok:
 call show_msg
 ret

; ----------------------------------------------------------------------
; printing: the PrintControl sends this after its dialog.
; CX:DX = PrintControl, BP = GString to draw into.
; ----------------------------------------------------------------------
menu_print:
 cmp word [V_INIT],0
 jne .go
 mov bx,cx                        ; not ready: just finish the job
 mov si,dx
 mov ax,0x25a
 mov di,0x8080
 KCALL 169
 retf
.go:
 mov es,[V_LSEG]
 mov [es:P_PCH],cx
 mov [es:P_PCC],dx
 mov [es:P_GS],bp
 call build_report
 mov ax,0x6016                    ; margins -> AX left, CX top, DX right, BP bottom
 call pc_call
 mov es,[V_LSEG]
 mov [es:P_ML],ax
 mov [es:P_MT],cx
 add dx,ax
 add bp,cx
 push dx                          ; left + right
 push bp                          ; top + bottom
 mov ax,0x6012                    ; paper size -> CX, DX
 call pc_call
 pop bp
 pop bx
 mov es,[V_LSEG]
 mov [es:P_PPW],cx
 mov [es:P_PPH],dx
 sub cx,bx
 mov [es:P_PRW],cx
 mov ax,dx
 sub ax,bp
 sub ax,P_HDR
 jg .h
 mov ax,100
.h:
 xor dx,dx
 mov cx,P_LH
 div cx
 cmp ax,5
 jae .l
 mov ax,5
.l:
 mov [es:P_LPP],ax
 ; count the lines -> sheets
 mov word [es:P_DRAW],0
 mov word [es:P_LINE],0
 call p_walk
 mov es,[V_LSEG]
 mov ax,[es:P_LINE]
 add ax,[es:P_LPP]
 dec ax
 xor dx,dx
 div word [es:P_LPP]
 cmp ax,1
 jae .n1
 mov ax,1
.n1:
 cmp ax,P_MAXP
 jbe .n2
 mov ax,P_MAXP
.n2:
 mov [es:P_NP],ax
 mul word [es:P_PPH]
 mov dx,ax
 mov cx,[es:P_PPW]
 mov ax,0x6009                    ; document size
 call pc_call
 ; origin: printable area of sheet 1
 mov es,[V_LSEG]
 mov di,[es:P_GS]
 mov dx,[es:P_ML]
 xor cx,cx
 mov bx,[es:P_MT]
 xor ax,ax
 KCALL 310                        ; GrApplyTranslation
 mov es,[V_LSEG]
 mov word [es:P_DRAW],1
 mov word [es:P_LINE],0
 mov word [es:P_PAGE],1
 call p_header
 call p_walk
 mov ax,0x25a                     ; printing done
 call pc_call
 mov ax,m_printed
 call show_msg
 retf

; all report lines: page titles, headings, rows
p_walk:
 xor cx,cx
.page:
 push cx
 or cx,cx
 jz .t
 mov al,3                         ; blank line
 call p_emit
.t:
 pop cx
 push cx
 mov bx,cx
 shl bx,1
 mov si,[cs:tab_names+bx]
 mov al,2
 call p_emit
 pop cx
 mov es,[V_LSEG]
 mov si,L_REP
.rec:
 mov es,[V_LSEG]
 mov al,[es:si]
 cmp al,0xff
 je .pend
 cmp al,cl
 je .mine
 call skip_rec
 jmp .rec
.mine:
 push cx
 mov al,[es:si+1]
 add si,2
 mov bx,si
 push si
 call skip_str
 xchg bx,si                       ; SI label, BX value
 call p_emit
 pop si
 mov es,[V_LSEG]
 call skip_str
 call skip_str
 pop cx
 jmp .rec
.pend:
 inc cx
 cmp cx,6
 jb .page
 ret

; AL = kind (0 row, 1 heading, 2 page title from CS, 3 blank),
; SI = label (state block or CS), BX = value
p_emit:
 mov es,[V_LSEG]
 cmp word [es:P_DRAW],0
 jne .draw
 inc word [es:P_LINE]
 ret
.draw:
 push ax
 mov ax,[es:P_LINE]
 cmp ax,[es:P_LPP]
 jb .same
 mov ax,[es:P_PAGE]
 cmp ax,[es:P_NP]
 jb .np
 pop ax
 ret                              ; beyond the last sheet
.np:
 push bx
 push si
 mov di,[es:P_GS]                 ; next sheet: one paper height down
 xor dx,dx
 xor cx,cx
 mov bx,[es:P_PPH]
 xor ax,ax
 KCALL 310
 mov es,[V_LSEG]
 inc word [es:P_PAGE]
 mov word [es:P_LINE],0
 call p_header
 pop si
 pop bx
.same:
 mov es,[V_LSEG]
 mov di,[es:P_GS]
 mov ax,[es:P_LINE]
 mov cx,P_LH
 mul cx
 add ax,P_HDR
 push ax
 inc word [es:P_LINE]
 pop dx                           ; y
 pop ax                           ; kind
 cmp al,3
 je .r
 push bx
 mov bx,dx
 cmp al,2
 jne .k1
 mov cx,F_SANS
 mov dx,12
 call set_font
 mov ax,0x0020
 call set_style
 mov al,C_DBLUE
 call set_tcol
 xor ax,ax
 call text_cs
 jmp .done
.k1:
 cmp al,1
 jne .k0
 mov cx,F_SANS
 mov dx,10
 call set_font
 mov ax,0x0020
 call set_style
 mov al,C_BLACK
 call set_tcol
 mov ax,8
 call text_ls
 jmp .done
.k0:
 mov cx,F_SANS
 mov dx,10
 call set_font
 mov ax,0xff00
 call set_style
 mov al,C_BLACK
 call set_tcol
 mov ax,16
 call text_ls
 pop si                           ; value
 push si
 mov ax,0x0020
 call set_style
 mov ax,190
 call text_ls
.done:
 mov ax,0xff00
 call set_style
 pop bx
.r:
 ret

; title, page number and a line at the top of a sheet (DI = GString)
p_header:
 mov es,[V_LSEG]
 mov di,[es:P_GS]
 mov cx,F_SANS
 mov dx,12
 call set_font
 mov ax,0x0020
 call set_style
 mov al,C_BLACK
 call set_tcol
 mov si,s_rtitle
 xor ax,ax
 xor bx,bx
 call text_cs
 mov ax,0xff00
 call set_style
 mov cx,F_SANS
 mov dx,9
 call set_font
 call v_reset
 mov si,s_rpage
 call v_s
 call v_sp
 mov es,[V_LSEG]
 mov ax,[es:P_PAGE]
 call v_n
 mov al,'/'
 call v_c
 mov ax,[es:P_NP]
 call v_n
 mov ax,[es:P_PRW]
 sub ax,60
 mov bx,2
 mov si,L_VAL
 call text_ls
 call v_reset
 call v_datetime
 mov ax,[es:P_PRW]
 sub ax,240
 mov si,L_VAL
 call text_ls
 mov al,C_DGREY
 call set_area
 xor ax,ax
 mov bx,18
 mov es,[V_LSEG]
 mov cx,[es:P_PRW]
 mov dx,19
 call fill
 ret

; the PrintControl asks its document object (us) for the document name:
; CX:DX = PrintControl, BP = reply message; reply with CX:DX = name
print_docname:
 cmp word [V_INIT],0
 jne .go
 retf
.go:
 mov es,[V_LSEG]
 mov [es:P_PCH],cx
 mov [es:P_PCC],dx
 push bp
 mov di,L_DOCN
 mov si,s_docname
.c:
 mov al,[cs:si]
 mov [es:di],al
 inc si
 inc di
 or al,al
 jnz .c
 pop ax                           ; reply message
 mov cx,[V_LSEG]
 mov dx,L_DOCN
 call pc_call
 retf

; page range 1..n for the print dialog (n estimated with 55 lines a sheet)
set_print_range:
 call build_report
 mov es,[V_LSEG]
 mov word [es:P_DRAW],0
 mov word [es:P_LINE],0
 call p_walk
 mov es,[V_LSEG]
 mov ax,[es:P_LINE]
 add ax,54
 xor dx,dx
 mov cx,55
 div cx
 cmp ax,1
 jae .r
 mov ax,1
.r:
 mov dx,ax
 push si
 push di
 push dx
 mov bx,3
 KCALL 150                        ; GeodeGetResourceHandle -> BX
 pop dx
 mov si,PC_CHUNK
 mov cx,1
 mov ax,0x6005
 xor di,di                        ; send, no reply needed
 KCALL 169                        ; ObjMessage
 pop di
 pop si
 ret

; ObjMessage (call) to the PrintControl: AX = message, CX DX BP = arguments
pc_call:
 push si
 push di
 mov es,[V_LSEG]
 mov bx,[es:P_PCH]
 mov si,[es:P_PCC]
 mov di,0x8080
 KCALL 169                        ; ObjMessage
 pop di
 pop si
 ret
