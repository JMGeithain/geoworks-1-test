; SPDX-License-Identifier: MIT
; GeoWorks 1.x Test - Systeminformation und SDK-Vorfuehrung fuer
; GeoWorks Ensemble 1.2 (Kernel 1.28), 8086, NASM, SDK pcgeos-1-sdk-v2.
;
; Seiten (Reiter): 0 Uebersicht, 1 System, 2 Speicher, 3 Bildschirm, 4 GEOS,
; 5 Leistung, 6 Grafik, 7 Eingabe, 8 Schriften, 9 Klang; dazu 10 Hilfe und
; 11 Info. (Tasten 1-9 und 0 waehlen die Reiter.)
; Die Seiten 0-5 sind ein "Bericht" aus Zeilen (Bezeichnung, Wert), der auch
; gespeichert (DOCUMENT\GWTEST.TXT), kopiert und gedruckt werden kann.
%include "ensemble12.inc"
%define PROCESS_CLASS 0x40
; Own Vis class (subclass of VisContentClass, UI export A3h, as KIDSDRAW):
; postbuild.pl places its class record at DGROUP 0x100 from the table
; "G1VISCLS" below, exports it as entry 0 and puts the object (chunk 22h)
; into resource 11; the view sends all its events to this object.
%define VIS_CLASS 0x100
%define VIS_RES   11
%define VIS_CHUNK 0x22
; ---- DGROUP: method table 0x52..0xCB (20 methods), variables 0xCC..0xFF ----
%define V_KROW    0xCC      ; sound page: button row / button of kb_rect
%define V_KBTN    0xCE
%define V_KX      0xD0      ; sound page: click position
%define V_KY      0xD2
%define V_ODS     0xD8      ; object block segment of the current Vis message
%define V_OSI     0xDA      ; its chunk
%define V_GRAB    0xDC      ; 1 = we hold the mouse grab
%define V_WIN     0xDE      ; 1 = view has been drawn once (window open)
%define V_INIT    0xE0
%define V_LSEG    0xE2      ; state block (locked for the app's life)
%define V_LHAN    0xE4
%define V_PAGE    0xE6      ; current page 0..11
%define V_VW      0xE8      ; window size in pixels
%define V_VH      0xEA
%define V_OX      0xEC      ; window origin (document coordinates)
%define V_OY      0xEE
%define V_TC      0xF0      ; 1 = true colour driver (GEOS.INI)
%define V_SCR     0xF2      ; first shown row of a report page
%define V_DOWN    0xF4      ; mouse button down (input page)
%define V_MX      0xF6      ; last mouse position
%define V_MY      0xF8
%define V_MSG     0xFA      ; status line: 0 hints, else CS offset of a message
%define V_SEED    0xFC
%define V_NROW    0xFE      ; rows of the shown report page

; ---- state block ----
%define L_VAL     0         ; 256 bytes: value being built
%define L_VP      256       ; write pointer into L_VAL
%define L_RP      258       ; write pointer into L_REP
%define L_RPG     260       ; page of the rows being added
%define L_XMSP    262       ; far pointer of the XMS driver
%define L_TICK    266       ; tick at the start of a measurement
%define L_CNT     268
%define L_CNTH    270
%define L_TW      272       ; tab width
%define L_PADX    274       ; input page: drawing pad
%define L_PADY    276
%define L_PADW    278
%define L_PADH    280
%define L_BTNX    282       ; performance page: button
%define L_BTNY    284
%define L_LBLW    286       ; label column width
%define L_FH      288       ; file handle (file test)
%define L_TMPW    290
%define L_TMPW2   292
%define L_TMPW3   294
%define L_SIG     296       ; CPUID signature (dword)
; raw results of the detection
%define H_CPU     300       ; 0 8086/8088, 1 80186/88, 2 80286, 3 386, 4 486+
%define H_CPUID   302       ; 1 = CPUID available
%define H_FAM     304
%define H_MOD     306
%define H_STEP    308
%define H_FEAT    310       ; CPUID 1 EDX (dword)
%define H_FPU     314
%define H_LOOPLO  316       ; loops per second
%define H_LOOPHI  318
%define H_SPD     320       ; 1 = measured
%define H_MODB    322       ; BIOS model byte
%define H_EMU     324       ; 1 = emulator recognised, 2 = BIOS maker only
%define H_EQ      326       ; INT 11h
%define H_CONV    328       ; INT 12h, KB
%define H_CMOS    330       ; extended memory from CMOS, KB
%define H_I15     332       ; INT 15h AH=88h, KB, 0xffff = none
%define H_XMS     334
%define H_XMSV    336
%define H_XMSR    338
%define H_XMSL    340
%define H_XMSF    342
%define H_EMS     344
%define H_EMSV    346
%define H_EMSF    348
%define H_EMST    350
%define H_DOSV    352       ; AL major, AH minor
%define H_DOSOEM  354
%define H_TRUEV   356       ; BL major BH minor (3306h)
%define H_DRV     358
%define H_DOK     360
%define H_DFREE   362       ; KB (dword)
%define H_DTOT    366       ; KB (dword)
%define H_WIN     370
%define H_VMODE   372
%define H_VCOLS   374
%define H_VBE     376
%define H_VBEV    378
%define H_VBEMEM  380
%define H_VMODEV  382
%define H_XRES    384
%define H_YRES    386
%define H_BPP     388
%define H_MIOK    390
%define H_HDD     392
%define H_KBD     394
%define H_COM     396       ; 4 words
%define H_LPT     404       ; 3 words
%define H_KSZLO   410
%define H_KSZHI   412
%define H_KOK     414
%define H_YEAR    416
%define H_MD      418       ; DH month, DL day
%define H_WD      420
%define H_HM      422       ; CH hour, CL minute
%define H_SEC     424
%define H_BENCH   426       ; 1 = graphics test done
%define B_RECT    428       ; operations per second
%define B_TEXT    430
%define B_ELL     432
%define B_BMP     434
%define B_MEM     436
%define B_WR      438       ; KB/s
%define B_RD      440
%define B_FOK     442
%define H_INIOK   444
%define H_NKEY    446       ; keys seen on the input page
%define H_CLICKS  448
%define H_CX      450
%define H_CY      452
%define H_KEYS    454       ; 8 x (CX, DX) = 32 bytes -> 486
%define H_SPDIX   486       ; speed index * 10
%define H_PAGES   488       ; estimated print pages
%define P_ML      490       ; printing
%define P_MT      492
%define P_PPW     494
%define P_PPH     496
%define P_LPP     498
%define P_LINE    500
%define P_PAGE    502
%define P_PCH     504
%define P_PCC     506
%define P_GS      508
%define P_NP      510
%define H_PSP     512
%define H_CPUIDMX 514
%define H_BDATE   520       ; 9 bytes
%define H_VENDOR  530       ; 13 bytes
%define H_BRAND   544       ; 49 bytes
%define H_BIOSS   594       ; 64 bytes
%define H_EMUN    658       ; 2: CS offset of the emulator name
%define H_VBEOEM  660       ; 48 bytes
%define H_VBEVEN  708       ; 40
%define H_VBEPRD  748       ; 40
%define H_COMSPEC 788       ; 64
%define H_BLASTER 852       ; 48
%define H_GPATH   900       ; 80
%define L_DOCN    980       ; 32: document name for the PrintControl
%define H_BP      1070      ; last button flags of MSG_META_PTR
%define H_ENDS    1072      ; MSG_META_END_SELECT seen
%define H_DRAGS   1074      ; MSG_META_DRAG_SELECT seen
%define L_LX      1076      ; last point of the stroke
%define L_LY      1078
%define L_HAVE    1080
%define L_NFONT   1082      ; font records
%define L_FDISK   1084      ; disk handle of GEOWORKS\FONT
%define L_FNAME   1086      ; name of the font file being read
%define L_SNDP    1088      ; sound: CS offset of the next note
%define L_SNDRUN  1090      ; 1 = a melody is playing
%define L_SNDBUSY 1092      ; 1 = inside snd_step
%define L_SNDRE   1094      ; 1 = the step message came back at once
%define L_INSTR   1096      ; instrument 0-3
%define L_SONG    1098      ; melody 0-3
%define L_TMP     1100      ; 512 scratch (VBE info)
%define L_MI      1612      ; 256 VBE mode info
%define L_INI     1868      ; GEOS.INI
%define INI_MAX   12000
%define L_REP     (L_INI+INI_MAX+16)
%define REP_MAX   8000
%define L_TXT     (L_REP+REP_MAX+16)
%define TXT_MAX   9000
%define L_FPATH   (L_TXT+TXT_MAX+16)   ; 128: path of GEOWORKS\FONT
%define L_FENT    (L_FPATH+128)        ; FileEnum entries (fonts.inc)
%define L_FONTS   (L_FENT+3304)        ; font records (fonts.inc)
%define L_SIZE    (L_FONTS+7216)

; ---- layout ----
%define HEAD_H    34
%define TAB_Y     40
%define TAB_H     20
%define TOP_Y     (TAB_Y+TAB_H+1)   ; content panel
%define ROW_Y     (TOP_Y+10)
%define ROW_H     15
%define FOOT_H    18
%define NPAGES    10
%define PG_FONTS  8
%define PG_KLANG  9
%define PG_HELP   10
%define PG_INFO   11
%define PC_CHUNK  0x40      ; PrintControl chunk in resource 3 (postbuild.pl)
%define MRF_PROCESSED 0x8000

; colours of the GEOS palette
%define C_BLACK   0
%define C_DBLUE   1
%define C_GREEN   2
%define C_CYAN    3
%define C_RED     4
%define C_GREY    7
%define C_DGREY   8
%define C_LBLUE   9
%define C_WHITE   15

; fonts
%define F_ROMAN   0x1000
%define F_SANS    0x1200
%define F_MONO    0x1a00
%define F_BERK    0x0202
%define F_UI      F_BERK    ; font of all screen texts (Berkeley: 9, 10, 12, 14, 18 pt)

G1_METHOD 0x0a01, app_open
G1_METHOD 0x0a03, app_close
G1_METHOD 0x0c00, menu_save
G1_METHOD 0x0c01, menu_copy
G1_METHOD 0x0c02, menu_print
G1_METHOD 0x0c03, menu_info
G1_METHOD 0x0c04, menu_help
G1_METHOD 0x0c05, menu_measure
G1_METHOD 0x0c06, snd_step
G1_METHOD 0x0259, print_docname
G1_METHOD 0x0c10, page_0
G1_METHOD 0x0c11, page_1
G1_METHOD 0x0c12, page_2
G1_METHOD 0x0c13, page_3
G1_METHOD 0x0c14, page_4
G1_METHOD 0x0c15, page_5
G1_METHOD 0x0c16, page_6
G1_METHOD 0x0c17, page_7
G1_METHOD 0x0c18, page_8
G1_METHOD 0x0c19, page_9

; ======================================================================
; lifecycle
; ======================================================================
app_open:
 push es
 pop ds
 mov di,PROCESS_CLASS
 KCALL 489                        ; ObjCallSuperNoLock
 ; detection here, in the process thread with a short call chain
 ; (not inside MSG_VIS_DRAW, which arrives deeply nested)
 push es
 pop ds
 cmp word [V_INIT],0
 jne .r
 mov word [V_VW],640
 mov word [V_VH],460
 call startup
.r:
 retf

app_close:
 push es
 pop ds
 call shutdown
 xor cx,cx
 retf

; ======================================================================
; the Vis class: message table read by postbuild.pl
; ======================================================================
vis_class_table:
 db 'G1VISCLS'
 dw 7
 dw 0x2071, 0x00c3, 0x00cb, 0x00c4, 0x007a, 0x007b, 0x0049
 dw vis_draw, start_select, drag_select, end_select, mouse_ptr, kbd_char, vis_exposed

; Vis messages arrive with *DS:SI = our object, ES:DI = our class
; (ES = DGROUP). Remember the object, then work with DS = DGROUP.
%macro VIS_ENTER 0
 mov [es:V_ODS],ds
 mov [es:V_OSI],si
 push es
 pop ds
%endmacro

; back to the caller with *DS:SI = our object again: the kernel unlocks
; the object block through DS after the method (AX = return value)
vis_ret:
 mov si,[V_OSI]
 mov ds,[V_ODS]
 retf

; pass the message in AX to VisContentClass (DS = DGROUP)
vis_super:
 push ds
 pop es
 mov si,[V_OSI]
 mov ds,[V_ODS]
 mov di,VIS_CLASS
 KCALL 489                        ; ObjCallSuperNoLock
 retf

; MSG_META_EXPOSED (49h): VisContentClass creates the gstate and sends
; MSG_VIS_DRAW to us; noted only to know that the window exists
vis_exposed:
 mov word [es:V_WIN],1
 VIS_ENTER
 mov ax,0x0049
 jmp vis_super

; MSG_VIS_DRAW (2071h): BP = gstate
vis_draw:
 VIS_ENTER
 push bp
 mov word [V_WIN],1
 mov di,bp
 cmp word [V_INIT],0
 jne .ready
 push di
 call get_bounds
 call startup
 pop di
.ready:
 push di
 call get_bounds
 call paint
 pop di
 pop bp
 jmp vis_ret

; the Vis bounds of our object = window size (left, top, right, bottom at
; the start of the Vis part), so the object always covers the whole view
fit_bounds:
 push ds
 push si
 push di
 mov ax,[V_VW]
 mov bx,[V_VH]
 dec ax
 dec bx
 mov si,[V_OSI]
 mov ds,[V_ODS]
 mov di,[si]
 add di,[di+4]
 mov word [di],0
 mov word [di+2],0
 mov [di+4],ax
 mov [di+6],bx
 pop di
 pop si
 pop ds
 ret

; UI export 213 / 204 on our object, as KIDSDRAW calls them in its
; START_SELECT and END_SELECT: take and give back the mouse
grab_mouse:
 cmp word [V_GRAB],0
 jne .r
 mov word [V_GRAB],1
 push ds
 push si
 mov si,[V_OSI]
 mov ds,[V_ODS]
 LC 0,213
 pop si
 pop ds
.r:
 ret
release_mouse:
 cmp word [V_GRAB],0
 je .r
 mov word [V_GRAB],0
 push ds
 push si
 mov si,[V_OSI]
 mov ds,[V_ODS]
 LC 0,204
 pop si
 pop ds
.r:
 ret

; allocate the state block, read GEOS.INI, detect the machine
startup:
 mov ax,L_SIZE
 mov cx,0xc000                    ; zero-initialised, locked
 KCALL 0                          ; MemAlloc -> AX segment, BX handle
 jnc .ok
 ret
.ok:
 mov [V_LSEG],ax
 mov [V_LHAN],bx
 mov word [V_SEED],12345
 mov word [V_MSG],0
 call load_ini
 call detect_tc
 call gather
 call font_scan
 mov word [V_INIT],1
 call set_print_range
 ret

shutdown:
 call spk_off
 cmp word [V_INIT],0
 je .r
 mov word [V_INIT],0
 mov bx,[V_LHAN]
 KCALL 2                          ; MemFree
 mov word [V_LSEG],0
.r:
 ret

; ======================================================================
; drawing framework
; ======================================================================
repaint:
 call begin_direct
 jc .no
 call paint
 call end_direct
.no:
 ret

; gstate for drawing outside an exposed event -> DI, CF = no window
begin_direct:
 cmp word [V_WIN],0
 je .no
 cmp word [V_INIT],0
 je .no
 push si
 push bp
 mov bx,VIS_RES
 KCALL 150                        ; GeodeGetResourceHandle
 mov si,VIS_CHUNK
 mov ax,0x2063                    ; MSG_VIS_VUP_CREATE_GSTATE -> BP
 mov di,0x8080                    ; call (as KIDSDRAW)
 KCALL 169                        ; ObjMessage
 mov di,bp
 pop bp
 pop si
 or di,di
 jz .no
 call get_bounds
 clc
 ret
.no:
 stc
 ret
end_direct:
 KCALL 212
 ret

; DI = gstate: window origin and size, origin -> 0,0
get_bounds:
 push di
 KCALL 440                        ; GrGetWinBounds
 pop di
 mov [V_OX],ax
 mov [V_OY],bx
 push ax
 push bx
 push cx
 push dx
 mov si,ax
 or si,bx
 jz .not
 mov dx,ax
 xor cx,cx
 xor ax,ax
 push di
 KCALL 310                        ; GrApplyTranslation
 pop di
.not:
 pop dx
 pop cx
 pop bx
 pop ax
 sub cx,ax
 sub dx,bx
 inc cx
 inc dx
 mov [V_VW],cx
 mov [V_VH],dx
 ret

; the whole window: frame, tabs, page, status line
paint:
 cmp word [V_INIT],0
 jne .go
 mov al,C_WHITE
 call set_area
 xor ax,ax
 xor bx,bx
 mov cx,[V_VW]
 mov dx,[V_VH]
 call fill
 ret
.go:
 call get_dt
 call build_report
 call draw_frame
 mov ax,[V_PAGE]
 cmp ax,6
 jne .p7
 call draw_graphics
 jmp .foot
.p7:
 cmp ax,7
 jne .p8
 call draw_input
 jmp .foot
.p8:
 cmp ax,PG_FONTS
 jne .p9k
 call draw_fonts
 jmp .foot
.p9k:
 cmp ax,PG_KLANG
 jne .p10
 mov word [V_SCR],0
 call draw_report_page
 call draw_sound
 jmp .foot
.p10:
 cmp ax,PG_HELP
 jne .p9
 mov si,help_text
 call draw_lines
 jmp .foot
.p9:
 cmp ax,PG_INFO
 jne .rep
 mov si,info_text
 call draw_lines
 call draw_info_icon
 jmp .foot
.rep:
 call draw_report_page
 cmp word [V_PAGE],5
 jne .foot
 call draw_bench_button
.foot:
 call draw_footer
 ret

; header band, tabs and the white panel
draw_frame:
 mov al,C_DBLUE
 call set_area
 xor ax,ax
 xor bx,bx
 mov cx,[V_VW]
 mov dx,HEAD_H
 call fill
 ; small icon in the header
 mov ax,[V_VW]
 sub ax,58
 mov bx,2
 call draw_icon_at
 mov cx,F_UI
 mov dx,18
 call set_font
 mov ax,0x0020                    ; bold
 call set_style
 mov al,C_WHITE
 call set_tcol
 mov si,s_title
 mov ax,12
 mov bx,6
 call text_cs
 mov ax,0xff00
 call set_style
 mov cx,F_UI
 mov dx,10
 call set_font
 mov al,C_WHITE
 call set_tcol
 ; subtitle right-aligned before the icon (Berkeley 10: about 305 pixels)
 mov si,s_sub
 mov ax,[V_VW]
 sub ax,58+8+305
 cmp ax,250
 jge .sx
 mov cx,F_UI                      ; narrow window: 9 point
 mov dx,9
 call set_font
 mov ax,[V_VW]
 sub ax,58+8+275
 cmp ax,240
 jl .nosub
.sx:
 mov bx,13
 call text_cs
.nosub:
 ; background below the header
 mov al,C_GREY
 call set_area
 xor ax,ax
 mov bx,HEAD_H
 mov cx,[V_VW]
 mov dx,[V_VH]
 call fill
 ; tab width
 mov ax,[V_VW]
 sub ax,16
 xor dx,dx
 mov cx,NPAGES
 div cx
 cmp ax,56
 jge .t1
 mov ax,56
.t1:
 cmp ax,110
 jle .t2
 mov ax,110
.t2:
 mov es,[V_LSEG]
 mov [es:L_TW],ax
 cmp ax,76                        ; narrow tabs: smaller labels
 jge .t3
 mov cx,F_UI
 mov dx,9
 call set_font
.t3:
 xor si,si                        ; tab number
.tab:
 push si
 mov es,[V_LSEG]
 mov ax,si
 mul word [es:L_TW]
 add ax,8
 mov bx,TAB_Y
 mov cx,ax
 add cx,[es:L_TW]
 sub cx,2
 mov dx,TAB_Y+TAB_H
 cmp si,[V_PAGE]
 jne .inact
 inc dx                           ; the active tab joins the panel
 push ax
 mov al,C_WHITE
 call set_area
 pop ax
 call fill
 push ax
 mov al,C_DBLUE
 call set_tcol
 mov ax,0x0020
 call set_style
 pop ax
 jmp .lbl
.inact:
 push ax
 mov al,C_LBLUE
 call set_area
 pop ax
 call fill
 push ax
 mov al,C_WHITE
 call set_tcol
 mov ax,0xff00
 call set_style
 pop ax
.lbl:
 push ax
 mov al,C_DGREY
 call set_lcol
 pop ax
 push dx
 cmp si,[V_PAGE]
 jne .fr
 dec dx
.fr:
 call frame
 pop dx
 ; label
 add ax,6
 mov bx,TAB_Y+4
 pop si
 push si
 shl si,1
 mov si,[cs:tab_names+si]
 call text_cs
 pop si
 inc si
 cmp si,NPAGES
 jb .tab
 mov ax,0xff00
 call set_style
 ; panel
 mov al,C_WHITE
 call set_area
 mov ax,8
 mov bx,TOP_Y
 mov cx,[V_VW]
 sub cx,8
 mov dx,[V_VH]
 sub dx,FOOT_H+4
 call fill
 mov al,C_DGREY
 call set_lcol
 dec bx
 call frame
 ; the active tab has no line below
 mov es,[V_LSEG]
 mov ax,[V_PAGE]
 cmp ax,NPAGES
 jae .r
 mul word [es:L_TW]
 add ax,9
 mov cx,ax
 add cx,[es:L_TW]
 sub cx,4
 push ax
 mov al,C_WHITE
 call set_area
 pop ax
 mov bx,TOP_Y-1
 mov dx,TOP_Y+1
 call fill
.r:
 ret

draw_footer:
 mov al,C_GREY
 call set_area
 xor ax,ax
 mov bx,[V_VH]
 sub bx,FOOT_H
 mov cx,[V_VW]
 mov dx,[V_VH]
 call fill
 mov cx,F_UI
 mov dx,10
 call set_font
 mov ax,0xff00
 call set_style
 mov al,C_BLACK
 call set_tcol
 mov si,[V_MSG]
 or si,si
 jnz .msg
 mov si,[V_PAGE]
 shl si,1
 mov si,[cs:foot_texts+si]
.msg:
 mov ax,10
 mov bx,[V_VH]
 sub bx,FOOT_H-3
 call text_cs
 ret

; ======================================================================
; report pages: rows of the report block for V_PAGE
; record: page byte, kind byte (0 row, 1 heading), label NUL, value NUL
; ======================================================================
draw_report_page:
 mov cx,F_UI
 mov dx,10
 call set_font
 mov word [V_NROW],0
 mov es,[V_LSEG]
 mov si,L_REP
 mov bx,ROW_Y
 xor dx,dx                        ; rows of this page seen
.next:
 mov al,[es:si]
 cmp al,0xff
 jne .rec
 jmp .end
.rec:
 cmp al,[V_PAGE]
 je .mine
 call skip_rec
 jmp .next
.mine:
 inc word [V_NROW]
 inc dx
 cmp dx,[V_SCR]
 ja .show
 call skip_rec
 jmp .next
.show:
 ; still room?
 mov ax,[V_VH]
 sub ax,FOOT_H+4+ROW_H
 cmp bx,ax
 jle .room
 call skip_rec
 jmp .next
.room:
 push dx
 cmp byte [es:si+1],1
 jne .row
 ; heading: bold, dark blue, line below
 add si,2
 cmp bx,ROW_Y
 je .h0
 add bx,5
.h0:
 mov cx,F_UI
 mov dx,12
 call set_font
 mov ax,0x0020
 call set_style
 mov al,C_DBLUE
 call set_tcol
 mov ax,18
 call text_ls
 push bx
 mov al,C_LBLUE
 call set_area
 mov ax,18
 add bx,16
 mov cx,[V_VW]
 sub cx,18
 mov dx,bx
 inc dx
 call fill
 pop bx
 add bx,20
 mov cx,F_UI
 mov dx,10
 call set_font
 mov ax,0xff00
 call set_style
 call skip_str
 call skip_str
 pop dx
 jmp .next
.row:
 add si,2
 mov al,C_DGREY
 call set_tcol
 mov ax,24
 call text_ls
 call skip_str
 mov al,C_BLACK
 call set_tcol
 mov ax,0x0020
 call set_style
 mov ax,24
 mov es,[V_LSEG]
 add ax,[es:L_LBLW]
 call text_ls
 mov ax,0xff00
 call set_style
 call skip_str
 add bx,ROW_H
 pop dx
 jmp .next
.end:
 ; more rows below: hint
 mov ax,[V_NROW]
 cmp ax,[V_SCR]
 jbe .r
 ret
.r:
 ret

; ES:SI at a record -> next record
skip_rec:
 add si,2
 call skip_str
 call skip_str
 ret
skip_str:
 cmp byte [es:si],0
 je .e
 inc si
 jmp skip_str
.e:
 inc si
 ret

; button "Jetzt messen" on the performance page
draw_bench_button:
 mov es,[V_LSEG]
 mov ax,[V_VW]
 sub ax,180
 mov [es:L_BTNX],ax
 mov bx,[V_VH]
 sub bx,FOOT_H+34
 mov [es:L_BTNY],bx
 mov cx,ax
 add cx,160
 mov dx,bx
 add dx,22
 push ax
 mov al,C_LBLUE
 call set_area
 pop ax
 call fill
 push ax
 mov al,C_DBLUE
 call set_lcol
 pop ax
 call frame
 add ax,22
 add bx,4
 mov cx,F_UI
 mov dx,12
 call set_font
 mov ax,0x0020
 call set_style
 mov al,C_WHITE
 call set_tcol
 mov es,[V_LSEG]
 mov ax,[es:L_BTNX]
 add ax,22
 mov si,s_measure
 call text_cs
 mov ax,0xff00
 call set_style
 ret

; text pages (help, info): CS:SI = lines "type, text, 0", end 0xff
; type 0 text, 1 heading, 2 small gap, 3 indented text
draw_lines:
 push si                          ; count the lines (scroll limit)
 xor cx,cx
.cnt:
 cmp byte [cs:si],0xff
 je .cnd
 inc cx
 inc si
 call skip_cs
 jmp .cnt
.cnd:
 mov [V_NROW],cx
 pop si
 mov bx,ROW_Y
 mov dx,[V_SCR]
.l:
 mov al,[cs:si]
 cmp al,0xff
 je .r
 inc si
 or dx,dx
 jz .show
 dec dx
 call skip_cs
 jmp .l
.show:
 push dx
 mov ah,al
 mov ax,[V_VH]
 sub ax,FOOT_H+4+ROW_H
 cmp bx,ax
 jg .full
 mov al,[cs:si-1]
 cmp al,1
 jne .t
 push si
 mov cx,F_UI
 mov dx,12
 call set_font
 mov ax,0x0020
 call set_style
 mov al,C_DBLUE
 call set_tcol
 pop si
 add bx,4
 mov ax,18
 call text_cs
 add bx,18
 jmp .nx
.t:
 cmp al,2
 jne .t3
 add bx,6
 jmp .nx
.t3:
 push ax
 push si
 mov cx,F_UI
 mov dx,10
 call set_font
 mov ax,0xff00
 call set_style
 mov al,C_BLACK
 call set_tcol
 pop si
 pop ax
 mov cx,24
 cmp al,3
 jne .t0
 mov cx,44
.t0:
 mov ax,cx
 call text_cs
 add bx,ROW_H
.nx:
 call skip_cs
 pop dx
 jmp .l
.full:
 pop dx
.r:
 mov ax,0xff00
 call set_style
 ret
skip_cs:
 cmp byte [cs:si],0
 je .e
 inc si
 jmp skip_cs
.e:
 inc si
 ret

; ======================================================================
; drawing helpers: DI = gstate; all registers kept
; ======================================================================
set_area:                         ; AL = palette index
 push ax
 push di
 xor ah,ah
 KCALL 355                        ; GrSetAreaColor
 pop di
 pop ax
 ret
set_area_rgb:                     ; AL = red, BL = green, BH = blue
 push ax
 push di
 mov ah,0x80
 KCALL 355
 pop di
 pop ax
 ret
set_tcol:
 push ax
 push di
 xor ah,ah
 KCALL 359                        ; GrSetTextColor
 pop di
 pop ax
 ret
set_lcol:
 push ax
 push di
 xor ah,ah
 KCALL 346                        ; GrSetLineColor
 pop di
 pop ax
 ret
fill:                             ; AX,BX - CX,DX
 push ax
 push bx
 push cx
 push dx
 push si
 push di
 KCALL 336                        ; GrFillRect
 pop di
 pop si
 pop dx
 pop cx
 pop bx
 pop ax
 ret
frame:
 push ax
 push bx
 push cx
 push dx
 push si
 push di
 KCALL 316                        ; GrDrawRect
 pop di
 pop si
 pop dx
 pop cx
 pop bx
 pop ax
 ret
ellipse:
 push ax
 push bx
 push cx
 push dx
 push si
 push di
 KCALL 324                        ; GrDrawEllipse
 pop di
 pop si
 pop dx
 pop cx
 pop bx
 pop ax
 ret
set_font:                         ; CX = font, DX = size
 push ax
 push bx
 push cx
 push dx
 push si
 push di
 xor ah,ah
 KCALL 366                        ; GrSetFont
 pop di
 pop si
 pop dx
 pop cx
 pop bx
 pop ax
 ret
set_style:                        ; AL = set, AH = clear
 push ax
 push bx
 push cx
 push dx
 push si
 push di
 KCALL 362                        ; GrSetTextStyle
 pop di
 pop si
 pop dx
 pop cx
 pop bx
 pop ax
 ret
; CS:SI at AX,BX
text_cs:
 push ds
 push cs
 pop ds
 call text_ds
 pop ds
 ret
; state block:SI at AX,BX
text_ls:
 push ds
 mov ds,[V_LSEG]
 call text_ds
 pop ds
 ret
text_ds:
 push ax
 push bx
 push cx
 push dx
 push si
 push di
 push es
 xor cx,cx
 KCALL 330                        ; GrDrawText
 pop es
 pop di
 pop si
 pop dx
 pop cx
 pop bx
 pop ax
 ret

; the 48x30 programme icon (4 bit with mask, packed) at AX,BX
draw_icon_at:
 push ax
 push bx
 push cx
 push dx
 push si
 push di
 push ds
 push cs
 pop ds
 mov si,icon_bmp
 xor dx,dx                        ; no callback
 KCALL 326                        ; GrDrawBitmap
 pop ds
 pop di
 pop si
 pop dx
 pop cx
 pop bx
 pop ax
 ret

; ======================================================================
; events
; ======================================================================
page_0:
 xor ax,ax
 jmp short goto_page
page_1:
 mov ax,1
 jmp short goto_page
page_2:
 mov ax,2
 jmp short goto_page
page_3:
 mov ax,3
 jmp short goto_page
page_4:
 mov ax,4
 jmp short goto_page
page_5:
 mov ax,5
 jmp short goto_page
page_6:
 mov ax,6
 jmp short goto_page
page_7:
 mov ax,7
 jmp short goto_page
page_8:
 mov ax,8
 jmp short goto_page
page_9:
 mov ax,9
goto_page:
 call set_page
 retf
menu_help:
 mov ax,PG_HELP
 call set_page
 retf
menu_info:
 mov ax,PG_INFO
 call set_page
 retf

set_page:
 cmp ax,[V_PAGE]
 je .same
 call stop_song
.same:
 mov word [V_DOWN],0
 call release_mouse
 mov [V_PAGE],ax
 mov word [V_SCR],0
 mov word [V_MSG],0
 call repaint
 ret

; MSG_META_KBD_CHAR: CX = character (CH = 0FFh special keys), DL = flags
kbd_char:
 VIS_ENTER
 test dl,1                        ; key release
 jnz .super
 cmp word [V_INIT],0
 je .super
 cmp word [V_PAGE],7
 jne .nav
 ; input page: every key is shown (the drawing pad is kept)
 call record_key
 call keys_refresh
 jmp vis_ret
.nav:
 mov word [V_MSG],0
 cmp ch,0xff
 jne .ascii
 cmp cl,0x90                      ; cursor up
 je .up
 cmp cl,0x91                      ; cursor down
 je .down
 cmp cl,0x92                      ; right
 je .next
 cmp cl,0x93                      ; left
 je .prev
 jmp .super
.ascii:
 cmp cl,'0'
 jne .a1
 mov ax,9
 call set_page
 jmp vis_ret
.a1:
 cmp cl,'1'
 jb .a2
 cmp cl,'9'
 ja .a2
 mov al,cl
 sub al,'1'
 xor ah,ah
 call set_page
 jmp vis_ret
.a2:
 mov al,cl
 or al,0x20
 cmp al,'n'
 je .next
 cmp cl,' '
 je .next
 cmp al,'p'
 je .prev
 cmp al,'m'
 je .meas
 cmp al,'s'
 je .save
 cmp al,'k'
 je .copy
 cmp al,'h'
 je .help
 cmp cl,'?'
 je .help
 cmp al,'i'
 je .info
 cmp cl,0x1b
 je .esc
 cmp cl,'+'
 je .down
 cmp cl,'-'
 je .up
 jmp .super
.up:
 cmp word [V_SCR],0
 je .done
 dec word [V_SCR]
 call repaint
 jmp vis_ret
.down:
 mov ax,[V_SCR]
 inc ax
 cmp ax,[V_NROW]
 jae .done
 mov [V_SCR],ax
 call repaint
 jmp vis_ret
.next:
 mov ax,[V_PAGE]
 inc ax
 cmp ax,NPAGES
 jb .np
 xor ax,ax
.np:
 call set_page
 jmp vis_ret
.prev:
 mov ax,[V_PAGE]
 or ax,ax
 jnz .pp
 mov ax,NPAGES
.pp:
 dec ax
 cmp ax,NPAGES
 jb .pp2
 mov ax,NPAGES-1
.pp2:
 call set_page
 jmp vis_ret
.meas:
 call do_measure
 jmp vis_ret
.save:
 call do_save
 jmp vis_ret
.copy:
 call do_copy
 jmp vis_ret
.help:
 mov ax,PG_HELP
 call set_page
 jmp vis_ret
.info:
 mov ax,PG_INFO
 call set_page
 jmp vis_ret
.esc:
 xor ax,ax
 call set_page
 jmp vis_ret
.done:
 jmp vis_ret
.super:
 mov ax,0x007b
 jmp vis_super

; MSG_META_START_SELECT: CX,DX = position (document)
start_select:
 VIS_ENTER
 cmp word [V_INIT],0
 jne .go
 jmp .done
.go:
 sub cx,[V_OX]
 sub dx,[V_OY]
 mov [V_MX],cx
 mov [V_MY],dx
 mov word [V_MSG],0
 ; tab row?
 cmp dx,TAB_Y
 jl .nottab
 cmp dx,TAB_Y+TAB_H
 jg .nottab
 mov ax,cx
 sub ax,8
 jl .nottab
 mov es,[V_LSEG]
 xor dx,dx
 div word [es:L_TW]
 cmp ax,NPAGES
 jae .nottab
 call set_page
 jmp .done
.nottab:
 cmp word [V_PAGE],PG_KLANG
 jne .nk
 call sound_click
 jmp .done
.nk:
 cmp word [V_PAGE],5
 jne .ni
 ; measure button
 mov es,[V_LSEG]
 mov ax,[es:L_BTNX]
 cmp cx,ax
 jl .done
 add ax,160
 cmp cx,ax
 jg .done
 mov ax,[es:L_BTNY]
 cmp dx,ax
 jl .done
 add ax,22
 cmp dx,ax
 jg .done
 call do_measure
 jmp .done
.ni:
 cmp word [V_PAGE],7
 jne .done
 mov es,[V_LSEG]
 inc word [es:H_CLICKS]
 mov [es:H_CX],cx
 mov [es:H_CY],dx
 ; pressed in the pad: draw while the button is held; the object
 ; takes the mouse, so every movement and the release reach us
 call in_pad
 jc .st
 mov word [V_DOWN],1
 mov word [es:L_HAVE],0
 call grab_mouse
 call pad_dot
.st:
 call input_status
.done:
 mov ax,MRF_PROCESSED
 jmp vis_ret

; MSG_META_DRAG_SELECT (CBh): moving with the button held
drag_select:
 VIS_ENTER
 cmp word [V_INIT],0
 je .r
 mov es,[V_LSEG]
 inc word [es:H_DRAGS]
.r:
 mov ax,MRF_PROCESSED
 jmp vis_ret

; MSG_META_END_SELECT (C4h): button released
end_select:
 VIS_ENTER
 mov word [V_DOWN],0
 call release_mouse
 cmp word [V_INIT],0
 je .r
 mov es,[V_LSEG]
 inc word [es:H_ENDS]
 cmp word [V_PAGE],7
 jne .r
 call input_status
.r:
 mov ax,MRF_PROCESSED
 jmp vis_ret

; MSG_META_PTR (7Ah): CX,DX = position, BP = buttons (8104h = left down)
mouse_ptr:
 VIS_ENTER
 cmp word [V_INIT],0
 je .super
 cmp word [V_PAGE],7
 jne .super
 push cx
 push dx
 sub cx,[V_OX]
 sub dx,[V_OY]
 mov [V_MX],cx
 mov [V_MY],dx
 mov es,[V_LSEG]
 mov [es:H_BP],bp
 cmp word [V_DOWN],0
 je .nd
 test bp,0x803c                   ; still held?
 jnz .draw
 mov word [V_DOWN],0              ; released without END_SELECT
 call release_mouse
 jmp .nd
.draw:
 call pad_dot
 pop dx
 pop cx
 mov ax,0x8800                    ; as KIDSDRAW answers MSG_META_PTR
 jmp vis_ret
.nd:
 call input_status
 pop dx
 pop cx
.super:
 mov ax,0x007a
 jmp vis_super

menu_save:
 call do_save
 retf
menu_copy:
 call do_copy
 retf
menu_measure:
 call do_measure
 retf

; CX,DX inside the drawing pad? CF = outside
in_pad:
 push es
 mov es,[V_LSEG]
 cmp cx,[es:L_PADX]
 jle .out
 cmp dx,[es:L_PADY]
 jle .out
 push ax
 mov ax,[es:L_PADX]
 add ax,[es:L_PADW]
 cmp cx,ax
 pop ax
 jge .out
 push ax
 mov ax,[es:L_PADY]
 add ax,[es:L_PADH]
 cmp dx,ax
 pop ax
 jge .out
 pop es
 clc
 ret
.out:
 pop es
 stc
 ret

; status line message (CS offset in AX) and repaint
show_msg:
 mov [V_MSG],ax
 call repaint
 ret

%include "sysinfo.inc"
%include "ini.inc"
%include "report.inc"
%include "bench.inc"
%include "demo.inc"
%include "output.inc"
%include "fonts.inc"
%include "sound.inc"
%include "g1clip.inc"
%include "texts.inc"
%include "icon.inc"
G1_FINISH
