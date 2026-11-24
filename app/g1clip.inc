; SPDX-License-Identifier: MIT
; g1clip.inc - text to the clipboard on GeoWorks Ensemble 1.2 (Kernel 1.28,
; UI protocol 693.0) for the ensemble12 NASM adapter of pcgeos-1-sdk-v2.
;
; The sequence is the one the 1.28 text object (UI.GEO, VisText "Kopieren")
; uses; registers are set exactly as there. See CLIPBOARD.md.
;
;   UI export 135  ClipboardGetClipboardFile      -> BX = VM file
;   UI export 127  ClipboardRegisterItem          BX:AX = file:header, BP = 0
;   kernel 0   MemAlloc       kernel 4   (block flags, used around VMAttach)
;   kernel 6   MemUnlock      kernel 24  LMem heap init   kernel 25 LMemAlloc
;   kernel 28  LMemReAlloc    kernel 149 GeodeGetProcessHandle
;   kernel 398/399 (bracket each VMAttach)   kernel 411 VMAttach
;
; Transfer item (format list in a 43h byte header block):
;   +00 owner chunk  +02 owner handle  +04 flags 4000h  +06 name (NUL)
;   +27 format count  +29 format IDs (10 words)  +3D 6 bytes per format:
;   extra1, extra2, VM block.  Format 0 = text: an LMem block with a 24h byte
;   header whose word at +10h is the text chunk (raw text, no NUL, CR between
;   paragraphs); +12h..+18h are the run arrays (0 = one plain style).
;
; Usage:  DS:SI = text (GEOS character set), CX = length, CS:DX = item name
;         call g1_clip_text      ; CF = failed. All registers but AX kept.

g1_clip_text:
 push bx
 push cx
 push dx
 push si
 push di
 push bp
 push ds
 push es
 sub sp,16
 mov bp,sp
 mov [bp+0],ds                    ; source
 mov [bp+2],si
 mov [bp+4],cx
 mov [bp+6],dx                    ; name in CS
 push bp
 LC 0,135                         ; ClipboardGetClipboardFile -> BX
 pop bp
 mov [bp+8],bx
 ; ---- text block: LMem heap with a 24h byte header ----
 mov ax,0x24
 mov cl,0x50
 mov ch,0x60
 KCALL 0                          ; MemAlloc -> AX segment, BX handle
 jnc .tb
 jmp .fail
.tb:
 mov [bp+10],bx
 mov ds,ax
 mov dx,0x24
 mov ax,0
 mov cl,0x80
 mov ch,0x10
 push bp
 KCALL 24                         ; LMem heap init (DS = block)
 pop bp
 xor ax,ax
 xor cx,cx
 push bp
 KCALL 25                         ; LMemAlloc, size 0 -> AX chunk
 pop bp
 mov [0x10],ax
 mov word [0x12],0
 mov word [0x14],0
 mov word [0x16],0
 mov word [0x18],0
 mov cx,[bp+4]
 push bp
 KCALL 28                         ; LMemReAlloc AX = chunk, CX = size
 pop bp
 mov bx,[0x10]
 mov di,[bx]                      ; chunk data
 push ds
 pop es
 mov [bp+14],es
 mov ds,[bp+0]
 mov si,[bp+2]
 mov cx,[bp+4]
 cld
 rep movsb
 mov bx,[bp+10]
 mov ah,0x80
 mov dl,0
 mov dh,0x8
 push bp
 KCALL 4
 pop bp
 mov es,[bp+14]
 mov word [es:0],0
 mov bx,[bp+10]
 push bp
 KCALL 6                          ; MemUnlock
 pop bp
 mov cx,[bp+10]
 mov bx,[bp+8]
 mov ax,bx
 call .bracket
 xor ax,ax
 push bp
 KCALL 411                        ; VMAttach -> AX = VM block
 pop bp
 call .bracket
 mov [bp+12],ax
 ; ---- header block with the format list ----
 push bp
 KCALL 149                        ; GeodeGetProcessHandle -> BX
 pop bp
 push bx
 mov ax,0x43
 mov cl,0x50
 mov ch,0x60
 push bp
 KCALL 0                          ; MemAlloc
 pop bp
 pop dx
 jc .fail
 mov [bp+10],bx
 mov es,ax
 xor di,di
 mov cx,0x43
 xor al,al
 rep stosb
 mov word [es:0],0                ; owner: our process
 mov [es:2],dx
 mov word [es:4],0x4000
 mov si,[bp+6]
 mov di,6
 mov cx,32
.name:
 mov al,[cs:si]
 or al,al
 jz .nend
 mov [es:di],al
 inc si
 inc di
 loop .name
.nend:
 mov byte [es:di],0
 mov word [es:0x27],1             ; one format
 mov word [es:0x29],0             ; format 0 = text
 mov ax,[bp+12]
 mov [es:0x41],ax                 ; its VM block
 mov bx,[bp+10]
 push bp
 KCALL 6                          ; MemUnlock
 pop bp
 mov cx,[bp+10]
 mov bx,[bp+8]
 xor ax,ax
 call .bracket
 push bp
 KCALL 411                        ; VMAttach -> AX = header block
 pop bp
 call .bracket                    ; leaves BX as ClipboardRegisterItem wants it
 push bp
 xor bp,bp
 LC 0,127                         ; ClipboardRegisterItem BX:AX, BP = 0
 pop bp
 clc
 jmp .out
.fail:
 stc
.out:
 lea sp,[bp+16]
 pop es
 pop ds
 pop bp
 pop di
 pop si
 pop dx
 pop cx
 pop bx
 ret
; the 1.28 text object calls kernel 399 (AX = file) and 398 (BX = file)
; before and after every VMAttach
.bracket:
 push ax
 xchg ax,bx
 push bp
 KCALL 399
 pop bp
 xchg ax,bx
 push bp
 KCALL 398
 pop bp
 xchg ax,bx
 pop ax
 ret
