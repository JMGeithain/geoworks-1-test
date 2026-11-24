; SPDX-License-Identifier: MIT
; report.inc - value builder and the report (pages 0-5, 9) from the detected
; values and GEOS.INI. Rebuilt for every painting (fast).

tab_names:  dw t_p0,t_p1,t_p2,t_p3,t_p4,t_p5,t_p6,t_p7,t_p8,t_p9
foot_texts: dw f_p0,f_p1,f_p1,f_p1,f_p1,f_p5,f_p6,f_p7,f_p8,f_p9,f_help,f_help

%macro HEAD 1
 mov si,%1
 call r_head
%endmacro
%macro ROW 1                      ; label; value is in L_VAL
 mov si,%1
 call r_row
%endmacro
%macro VS 1
 push si
 mov si,%1
 call v_s
 pop si
%endmacro
%macro INIROW 3                   ; label, section, key
 mov si,%2
 mov bx,%3
 call ini_get_or_dash
 mov si,%1
 call r_row
%endmacro

; ----------------------------------------------------------------------
; value builder: L_VAL with write pointer L_VP. All registers kept.
; ----------------------------------------------------------------------
v_reset:
 push es
 mov es,[V_LSEG]
 mov word [es:L_VP],L_VAL
 mov byte [es:L_VAL],0
 pop es
 ret
v_c:                              ; AL
 push di
 push es
 mov es,[V_LSEG]
 mov di,[es:L_VP]
 cmp di,L_VAL+250
 jae .f
 mov [es:di],al
 inc di
 mov byte [es:di],0
 mov [es:L_VP],di
.f:
 pop es
 pop di
 ret
v_s:                              ; CS:SI
 push ax
 push si
.l:
 mov al,[cs:si]
 or al,al
 jz .e
 call v_c
 inc si
 jmp .l
.e:
 pop si
 pop ax
 ret
v_ls:                             ; state block:SI
 push ax
 push si
 push es
 mov es,[V_LSEG]
.l:
 mov al,[es:si]
 or al,al
 jz .e
 call v_c
 inc si
 jmp .l
.e:
 pop es
 pop si
 pop ax
 ret
v_sp:
 push ax
 mov al,' '
 call v_c
 pop ax
 ret
v_n:                              ; AX unsigned, no thousands dots
 push ax
 push bx
 push cx
 push dx
 xor cx,cx
 mov bx,10
.d:
 xor dx,dx
 div bx
 push dx
 inc cx
 or ax,ax
 jnz .d
.o:
 pop ax
 add al,'0'
 call v_c
 loop .o
 pop dx
 pop cx
 pop bx
 pop ax
 ret
v_n2:                             ; AX as two digits
 push ax
 push bx
 mov bl,10
 div bl
 add ax,'00'
 call v_c
 mov al,ah
 call v_c
 pop bx
 pop ax
 ret
v_dec1:                           ; AX = value * 10 -> "x,y"
 push ax
 push bx
 push dx
 xor dx,dx
 mov bx,10
 div bx
 push dx
 xor dx,dx
 call v_nt
 mov al,','
 call v_c
 pop ax
 add al,'0'
 call v_c
 pop dx
 pop bx
 pop ax
 ret
v_hex:                            ; AX, CL digits
 push ax
 push bx
 push cx
 push dx
 mov dx,ax
 mov ch,cl
.d:
 dec ch
 js .e
 mov ax,dx
 push cx
 mov cl,ch
 shl cl,1
 shl cl,1
 shr ax,cl
 pop cx
 and al,15
 add al,'0'
 cmp al,'9'
 jbe .o
 add al,7
.o:
 call v_c
 jmp .d
.e:
 pop dx
 pop cx
 pop bx
 pop ax
 ret
v_hexh:                           ; "xxxxh"
 call v_hex
 push ax
 mov al,'h'
 call v_c
 pop ax
 ret
; DX:AX / 10 -> DX:AX, remainder BX
div32_10:
 push cx
 mov cx,10
 mov bx,ax
 mov ax,dx
 xor dx,dx
 div cx
 xchg ax,bx
 div cx
 xchg dx,bx
 pop cx
 ret
v_nt:                             ; DX:AX unsigned, with thousands dots
 push ax
 push bx
 push cx
 push dx
 push si
 xor cx,cx
.d:
 call div32_10
 push bx
 inc cx
 mov si,ax
 or si,dx
 jnz .d
.o:
 pop ax
 add al,'0'
 call v_c
 dec cx
 jz .e
 mov ax,cx
 mov bl,3
 div bl
 or ah,ah
 jnz .o
 mov al,'.'
 call v_c
 jmp .o
.e:
 pop si
 pop dx
 pop cx
 pop bx
 pop ax
 ret
v_kb:                             ; DX:AX KB -> "n KB (x,y MB)"
 push ax
 push bx
 push cx
 push dx
 call v_nt
 VS v_kb_s
 or dx,dx
 jnz .mb
 cmp ax,1024
 jb .e
.mb:
 mov bx,ax
 and bx,1023                      ; KB below a full MB
 mov cx,10
.q:
 shr dx,1
 rcr ax,1
 loop .q                          ; AX = whole MB
 or dx,dx
 jnz .e
 cmp ax,6000
 ja .e
 push ax
 mov ax,bx
 mov cx,10
 mul cx
 mov cx,1024
 div cx                           ; tenth of a MB
 mov bx,ax
 pop ax
 mov cx,10
 mul cx
 add ax,bx
 push ax
 mov al,' '
 call v_c
 mov al,'('
 call v_c
 pop ax
 call v_dec1
 VS v_mb_s
 mov al,')'
 call v_c
.e:
 pop dx
 pop cx
 pop bx
 pop ax
 ret
v_kb_s: db ' KB',0
v_mb_s: db ' MB',0

; ----------------------------------------------------------------------
; report records: page, kind (0 row, 1 heading), label NUL, value NUL
; ----------------------------------------------------------------------
r_begin:
 mov es,[V_LSEG]
 mov word [es:L_RP],L_REP
 mov byte [es:L_REP],0xff
 call v_reset
 ret
r_page:                           ; AL
 mov es,[V_LSEG]
 mov [es:L_RPG],al
 ret
r_head:                           ; CS:SI
 push ax
 push di
 push si
 push es
 mov es,[V_LSEG]
 mov di,[es:L_RP]
 cmp di,L_REP+REP_MAX-200
 jae .f
 mov al,[es:L_RPG]
 mov [es:di],al
 mov byte [es:di+1],1
 add di,2
.c:
 mov al,[cs:si]
 mov [es:di],al
 inc si
 inc di
 or al,al
 jnz .c
 mov byte [es:di],0
 inc di
 mov byte [es:di],0xff
 mov [es:L_RP],di
.f:
 pop es
 pop si
 pop di
 pop ax
 call v_reset
 ret
r_row:                            ; CS:SI label, L_VAL value
 push ax
 push di
 push si
 push es
 mov es,[V_LSEG]
 mov di,[es:L_RP]
 cmp di,L_REP+REP_MAX-300
 jae .f
 mov al,[es:L_RPG]
 mov [es:di],al
 mov byte [es:di+1],0
 add di,2
.c:
 mov al,[cs:si]
 mov [es:di],al
 inc si
 inc di
 or al,al
 jnz .c
 mov si,L_VAL
.v:
 mov al,[es:si]
 mov [es:di],al
 inc si
 inc di
 or al,al
 jnz .v
 mov byte [es:di],0xff
 mov [es:L_RP],di
.f:
 pop es
 pop si
 pop di
 pop ax
 call v_reset
 ret

; ----------------------------------------------------------------------
; pieces used on several pages
; ----------------------------------------------------------------------
v_cpuname:
 mov es,[V_LSEG]
 mov ax,[es:H_CPU]
 cmp word [es:H_CPUID],0
 je .nocpuid
 mov bx,[es:H_FAM]
 mov si,v_cpu4
 cmp bx,4
 jbe .s
 mov si,v_cpu5
 cmp bx,5
 je .s
 mov si,v_cpu6
 cmp bx,15
 jb .s
 mov si,v_cpu15
 jmp .s
.nocpuid:
 mov bx,ax
 shl bx,1
 mov si,[cs:cpu_names+bx]
.s:
 call v_s
 ret
cpu_names: dw v_cpu0,v_cpu1,v_cpu2,v_cpu3,v_cpu4

v_emuname:
 mov es,[V_LSEG]
 cmp word [es:H_EMU],0
 jne .k
 VS v_noemu
 ret
.k:
 mov si,[es:H_EMUN]
 call v_s
 ret

v_speed:
 mov es,[V_LSEG]
 cmp word [es:H_SPD],0
 jne .k
 VS v_notm
 ret
.k:
 mov ax,[es:H_SPDIX]
 call v_dec1
 call v_sp
 VS v_xt
 ret

v_ram:
 mov es,[V_LSEG]
 mov ax,[es:H_CONV]
 xor dx,dx
 call v_nt
 VS v_kb_s
 call v_sp
 VS v_conv
 mov es,[V_LSEG]
 mov ax,[es:H_CMOS]
 or ax,ax
 jz .r
 mov al,' '
 call v_c
 mov al,'+'
 call v_c
 call v_sp
 mov es,[V_LSEG]
 mov ax,[es:H_CMOS]
 xor dx,dx
 call v_nt
 VS v_kb_s
 call v_sp
 VS v_ext
.r:
 ret

v_resolution:
 mov es,[V_LSEG]
 cmp word [es:H_MIOK],0
 jne .k
 VS s_dash
 ret
.k:
 mov ax,[es:H_XRES]
 call v_n
 VS s_times
 mov es,[V_LSEG]
 mov ax,[es:H_YRES]
 call v_n
 ret
s_times: db ' x ',0

v_depth:                          ; "16 Bit (65.536 Farben)"
 mov es,[V_LSEG]
 mov cx,[es:H_BPP]
 jcxz .r
 mov ax,cx
 call v_n
 call v_sp
 VS v_bit
 cmp cx,24
 ja .r
 mov al,' '
 call v_c
 mov al,'('
 call v_c
 mov ax,1
 xor dx,dx
.p:
 shl ax,1
 rcl dx,1
 loop .p
 call v_nt
 call v_sp
 VS v_colors
 mov al,')'
 call v_c
.r:
 ret

v_dosver:
 mov es,[V_LSEG]
 mov al,[es:H_DOSV]
 xor ah,ah
 call v_n
 mov al,'.'
 call v_c
 mov al,[es:H_DOSV+1]
 xor ah,ah
 call v_n2
 call v_sp
 mov al,[es:H_DOSOEM]
 mov si,v_msdos
 cmp al,0xff
 je .s
 mov si,v_pcdos
 cmp al,0
 je .s
 mov si,v_drdos
 cmp al,0xee
 je .s
 mov si,v_freedos
 cmp al,0xfd
 je .s
 VS v_oem
 call v_sp
 xor ah,ah
 mov cl,2
 call v_hexh
 ret
.s:
 call v_s
 ret

v_geosver:
 mov si,s_swelcome
 mov bx,s_kversion
 call ini_get_or_dash
 ret

v_datetime:
 mov es,[V_LSEG]
 mov bx,[es:H_WD]
 cmp bx,6
 ja .nd
 shl bx,1
 mov al,[cs:v_days+bx]
 call v_c
 mov al,[cs:v_days+bx+1]
 call v_c
 call v_sp
.nd:
 mov al,[es:H_MD]                 ; day
 xor ah,ah
 call v_n2
 mov al,'.'
 call v_c
 mov al,[es:H_MD+1]
 xor ah,ah
 call v_n2
 mov al,'.'
 call v_c
 mov ax,[es:H_YEAR]
 call v_n
 mov al,','
 call v_c
 call v_sp
 mov al,[es:H_HM+1]
 xor ah,ah
 call v_n2
 mov al,':'
 call v_c
 mov al,[es:H_HM]
 xor ah,ah
 call v_n2
 mov al,':'
 call v_c
 mov al,[es:H_SEC+1]
 xor ah,ah
 call v_n2
 call v_sp
 VS v_uhr
 ret

v_ops:                            ; AX per second
 xor dx,dx
 call v_nt
 call v_sp
 VS v_persec
 ret

; ----------------------------------------------------------------------
; build the report
; ----------------------------------------------------------------------
build_report:
 call r_begin
 mov es,[V_LSEG]
 mov word [es:L_LBLW],215
 ; ================= page 0: overview =================
 mov al,0
 call r_page
 HEAD h_glance
 call v_emuname
 ROW l_machine
 call v_cpuname
 ROW l_cpu
 mov es,[V_LSEG]
 mov si,v_yes
 cmp word [es:H_FPU],0
 jne .f0
 mov si,v_no
.f0:
 call v_s
 ROW l_fpu
 call v_speed
 ROW l_speed
 call v_ram
 ROW l_ram
 call v_resolution
 mov es,[V_LSEG]
 cmp word [es:H_BPP],0
 je .g0
 VS s_commasp
 call v_depth
.g0:
 ROW l_gfx
 call v_geosver
 ROW l_geos
 call v_dosver
 ROW l_dos
 call v_datetime
 ROW l_date
 HEAD h_app
 mov ax,[V_VW]
 call v_n
 VS s_times
 mov ax,[V_VH]
 call v_n
 ROW l_wsize
 ; ================= page 1: system =================
 mov al,1
 call r_page
 HEAD h_cpu
 call v_cpuname
 ROW l_type
 mov es,[V_LSEG]
 cmp word [es:H_CPUID],0
 je .nocpuid
 mov si,H_VENDOR
 call v_ls
 ROW l_vendor
 mov es,[V_LSEG]
 mov ax,[es:H_FAM]
 call v_n
 VS s_slash
 mov ax,[es:H_MOD]
 call v_n
 VS s_slash
 mov ax,[es:H_STEP]
 call v_n
 ROW l_fms
 mov es,[V_LSEG]
 cmp byte [es:H_BRAND],0
 je .nocpuid
 mov si,H_BRAND
.bsp:
 cmp byte [es:si],' '             ; brand strings often start with blanks
 jne .bs
 inc si
 jmp .bsp
.bs:
 call v_ls
 ROW l_brand
.nocpuid:
 mov es,[V_LSEG]
 mov si,v_yes
 cmp word [es:H_FPU],0
 jne .f1
 mov si,v_no
.f1:
 call v_s
 mov es,[V_LSEG]
 test byte [es:H_FEAT],1
 jz .f2
 VS s_commasp
 VS v_integr
.f2:
 ROW l_fpu
 mov es,[V_LSEG]
 cmp word [es:H_SPD],0
 je .ns
 mov ax,[es:H_LOOPLO]
 mov dx,[es:H_LOOPHI]
 call v_nt
 ROW l_loops
.ns:
 call v_speed
 ROW l_index
 HEAD h_bios
 call v_emuname
 ROW l_emu
 mov es,[V_LSEG]
 cmp byte [es:H_BIOSS],0
 je .nb
 mov si,H_BIOSS
 call v_ls
 ROW l_bios
.nb:
 mov si,H_BDATE
 call v_ls
 ROW l_bdate
 mov es,[V_LSEG]
 mov ax,[es:H_MODB]
 push ax
 mov cl,2
 call v_hexh
 call v_sp
 mov al,'('
 call v_c
 pop ax
 mov si,v_mb0
 sub al,0xf8
 jb .mb
 cmp al,7
 ja .mb
 xor ah,ah
 mov bx,ax
 shl bx,1
 mov si,[cs:mb_names+bx]
.mb:
 call v_s
 mov al,')'
 call v_c
 ROW l_modb
 HEAD h_equip
 mov es,[V_LSEG]
 mov ax,[es:H_EQ]
 xor bx,bx
 test al,1
 jz .fl
 mov bl,al
 mov cl,6
 shr bl,cl
 inc bx
.fl:
 mov ax,bx
 call v_n
 ROW l_floppy
 mov es,[V_LSEG]
 mov ax,[es:H_HDD]
 call v_n
 ROW l_hdd
 mov bx,H_COM
 mov cx,4
 mov dl,'C'
 call v_ports
 ROW l_com
 mov bx,H_LPT
 mov cx,3
 mov dl,'L'
 call v_ports
 ROW l_lpt
 mov es,[V_LSEG]
 mov si,v_ext101
 test byte [es:H_KBD],0x10
 jnz .kb
 mov si,v_std84
.kb:
 call v_s
 ROW l_kbd
 HEAD h_dos
 call v_dosver
 ROW l_dosver
 mov es,[V_LSEG]
 mov ax,[es:H_TRUEV]
 or ax,ax
 jz .nt
 mov al,ah
 push ax
 mov al,[es:H_TRUEV]
 xor ah,ah
 call v_n
 mov al,'.'
 call v_c
 pop ax
 xor ah,ah
 call v_n2
 ROW l_truev
.nt:
 mov es,[V_LSEG]
 mov al,[es:H_DRV]
 add al,'A'
 call v_c
 mov al,':'
 call v_c
 ROW l_drive
 mov es,[V_LSEG]
 cmp word [es:H_DOK],0
 je .nd
 mov ax,[es:H_DFREE]
 mov dx,[es:H_DFREE+2]
 call v_kb
 call v_sp
 VS v_free
 call v_sp
 VS v_of
 call v_sp
 mov es,[V_LSEG]
 mov ax,[es:H_DTOT]
 mov dx,[es:H_DTOT+2]
 call v_kb
 ROW l_space
.nd:
 mov es,[V_LSEG]
 mov ax,[es:H_WIN]
 mov si,v_winno
 or al,al
 jz .w0
 cmp al,0x80
 je .w0
 VS v_win
 call v_sp
 mov es,[V_LSEG]
 mov ax,[es:H_WIN]
 cmp al,1
 je .w2
 cmp al,0xff
 je .w2
 push ax
 xor ah,ah
 call v_n
 mov al,'.'
 call v_c
 pop ax
 mov al,ah
 xor ah,ah
 call v_n2
 jmp .w1
.w2:
 mov al,'2'
 call v_c
 jmp .w1
.w0:
 call v_s
.w1:
 ROW l_windows
 mov si,H_COMSPEC
 call v_ls_or
 ROW l_comspec
 mov si,H_BLASTER
 call v_ls_or
 ROW l_blaster
 ; ================= page 2: memory =================
 mov al,2
 call r_page
 HEAD h_conv
 mov es,[V_LSEG]
 mov ax,[es:H_CONV]
 xor dx,dx
 call v_kb
 ROW l_int12
 HEAD h_ext
 mov es,[V_LSEG]
 mov ax,[es:H_CMOS]
 cmp word [es:H_CPU],2
 jae .cm
 VS v_noat
 jmp .cm2
.cm:
 xor dx,dx
 call v_kb
.cm2:
 ROW l_cmos
 mov es,[V_LSEG]
 mov ax,[es:H_I15]
 cmp ax,0xffff
 jne .i1
 VS v_nosup
 jmp .i3
.i1:
 or ax,ax
 jnz .i2
 cmp word [es:H_XMS],0
 je .i2
 VS v_xmsby
 jmp .i3
.i2:
 xor dx,dx
 call v_kb
.i3:
 ROW l_int15
 HEAD h_xms
 mov es,[V_LSEG]
 cmp word [es:H_XMS],0
 jne .x1
 VS v_no
 ROW l_drv
 jmp .ems
.x1:
 VS v_yes
 VS s_commasp
 VS v_xmsv
 call v_sp
 mov es,[V_LSEG]
 mov ax,[es:H_XMSV]
 call v_bcd
 VS s_commasp
 VS v_xmsdrv
 call v_sp
 mov es,[V_LSEG]
 mov ax,[es:H_XMSR]
 call v_bcd
 ROW l_drv
 mov es,[V_LSEG]
 mov ax,[es:H_XMSF]
 xor dx,dx
 call v_kb
 ROW l_xfree
 mov es,[V_LSEG]
 mov ax,[es:H_XMSL]
 xor dx,dx
 call v_kb
 ROW l_xlarge
.ems:
 HEAD h_ems
 mov es,[V_LSEG]
 cmp word [es:H_EMS],0
 jne .e1
 VS v_no
 ROW l_drv
 jmp .gm
.e1:
 VS v_yes
 VS s_commasp
 VS v_emsv
 call v_sp
 mov es,[V_LSEG]
 mov ax,[es:H_EMSV]
 push ax
 mov cl,4
 shr ax,cl
 call v_n
 mov al,'.'
 call v_c
 pop ax
 and ax,15
 call v_n
 ROW l_drv
 mov es,[V_LSEG]
 mov ax,[es:H_EMSF]
 mov cx,16
 mul cx
 call v_kb
 call v_sp
 VS v_of
 call v_sp
 mov es,[V_LSEG]
 mov ax,[es:H_EMST]
 mov cx,16
 mul cx
 call v_kb
 ROW l_efree
.gm:
 HEAD h_gmem
 INIROW l_gmemdrv,s_ssystem,s_kmemory
 INIROW l_handles,s_ssystem,s_khandles
 INIROW l_swap,s_sdiskswap,s_kfile
 mov si,s_sdiskswap
 mov bx,s_ksize
 call ini_get
 jc .nsw
 VS v_kb_s
 ROW l_swsize
.nsw:
 call v_reset
 ; ================= page 3: screen =================
 mov al,3
 call r_page
 HEAD h_gscr
 INIROW l_device,s_sscreen,s_kdevice
 INIROW l_drv,s_sscreen,s_kdriver
 mov si,v_16
 cmp word [V_TC],0
 je .tc
 mov si,v_tc
.tc:
 call v_s
 ROW l_cmode
 HEAD h_card
 mov es,[V_LSEG]
 mov ax,[es:H_VMODEV]
 mov cl,2
 cmp ax,0xff
 jbe .vm
 mov cl,3
.vm:
 call v_hexh
 ROW l_vmode
 call v_resolution
 ROW l_res
 mov es,[V_LSEG]
 cmp word [es:H_BPP],0
 je .nbpp
 call v_depth
 ROW l_depth
.nbpp:
 mov es,[V_LSEG]
 cmp word [es:H_VBE],0
 jne .vb
 VS v_no
 ROW l_vbe
 jmp .win
.vb:
 VS v_vbe
 call v_sp
 mov es,[V_LSEG]
 mov ax,[es:H_VBEV]
 push ax
 mov al,ah
 xor ah,ah
 call v_n
 mov al,'.'
 call v_c
 pop ax
 xor ah,ah
 call v_n
 ROW l_vbe
 mov si,H_VBEOEM
 call v_ls_or
 ROW l_oem
 mov es,[V_LSEG]
 cmp byte [es:H_VBEVEN],0
 je .nv
 mov si,H_VBEVEN
 call v_ls
 VS s_slashsp
 mov si,H_VBEPRD
 call v_ls
 ROW l_vprod
.nv:
 mov es,[V_LSEG]
 mov ax,[es:H_VBEMEM]
 mov cx,64
 mul cx
 call v_kb
 ROW l_vmem
.win:
 HEAD h_win
 mov ax,[V_VW]
 call v_n
 VS s_times
 mov ax,[V_VH]
 call v_n
 ROW l_size
 mov ax,[V_OX]
 call v_n
 VS s_commasp
 mov ax,[V_OY]
 call v_n
 ROW l_origin
 ; ================= page 4: GEOS =================
 mov al,4
 call r_page
 HEAD h_ver
 call v_geosver
 ROW l_geos
 VS v_proto
 ROW l_proto
 mov es,[V_LSEG]
 cmp word [es:H_KOK],0
 je .nk
 mov ax,[es:H_KSZLO]
 mov dx,[es:H_KSZHI]
 call v_nt
 call v_sp
 VS v_bytes
 ROW l_kernel
.nk:
 mov es,[V_LSEG]
 mov al,[es:H_DRV]
 add al,'A'
 call v_c
 mov al,':'
 call v_c
 mov si,H_GPATH
 cmp byte [es:si],'\'
 je .gp
 mov al,'\'
 call v_c
.gp:
 call v_ls
 ROW l_dir
 HEAD h_ui
 INIROW l_gui,s_sui,s_kgeneric
 INIROW l_sui,s_sui,s_kspecific
 mov si,s_ssystem
 mov bx,s_kfontid
 call ini_get_or_dash
 call v_sp
 mov si,s_ssystem
 mov bx,s_kfontsize
 call ini_get
 ROW l_font
 INIROW l_backgr,s_sui,s_kbackground
 INIROW l_autost,s_sui,s_kexec
 HEAD h_dev
 INIROW l_kbd,s_skeyboard,s_kdevice
 INIROW l_kdrv,s_skeyboard,s_kdriver
 INIROW l_mouse,s_smouse,s_kdevice
 INIROW l_mdrv,s_smouse,s_kdriver
 INIROW l_printer,s_sprinter,s_kprinters
 INIROW l_nprint,s_sprinter,s_knprint
 INIROW l_task,s_swelcome,s_ktask
 ; ================= page 5: performance =================
 mov al,5
 call r_page
 HEAD h_pcpu
 mov es,[V_LSEG]
 cmp word [es:H_SPD],0
 je .p1
 mov ax,[es:H_LOOPLO]
 mov dx,[es:H_LOOPHI]
 call v_nt
 ROW l_loops
.p1:
 call v_speed
 ROW l_index
 HEAD h_pgfx
 mov es,[V_LSEG]
 cmp word [es:H_BENCH],0
 jne .p2
 VS v_notyet
 ROW l_status
 jmp .pe
.p2:
 mov ax,[es:B_RECT]
 call v_ops
 ROW l_rect
 mov es,[V_LSEG]
 mov ax,[es:B_TEXT]
 call v_ops
 ROW l_text
 mov es,[V_LSEG]
 mov ax,[es:B_ELL]
 call v_ops
 ROW l_ell
 mov es,[V_LSEG]
 mov ax,[es:B_BMP]
 call v_ops
 ROW l_bmp
 HEAD h_psys
 mov es,[V_LSEG]
 mov ax,[es:B_MEM]
 call v_ops
 ROW l_mem
 mov es,[V_LSEG]
 cmp word [es:B_FOK],0
 je .pe
 mov ax,[es:B_WR]
 xor dx,dx
 call v_nt
 call v_sp
 VS v_kbps
 ROW l_fwr
 mov es,[V_LSEG]
 mov ax,[es:B_RD]
 xor dx,dx
 call v_nt
 call v_sp
 VS v_kbps
 ROW l_frd
.pe:
 call sound_rows
 ret

mb_names: dw v_mbf8,v_mbf9,v_mbfa,v_mbfb,v_mbfc,v_mbfd,v_mbfe,v_mbff

; "COM1 (3F8h), COM2 (2F8h)": BX = first address in the state block,
; CX = count, DL = 'C' or 'L'
v_ports:
 mov es,[V_LSEG]
 xor si,si                        ; found
 mov dh,'1'
.p:
 mov ax,[es:bx]
 or ax,ax
 jz .n
 push ax
 or si,si
 jz .f
 push si
 mov si,s_commasp
 call v_s
 pop si
.f:
 inc si
 push si
 mov si,s_com
 cmp dl,'C'
 je .c
 mov si,s_lpt
.c:
 call v_s
 pop si
 mov al,dh
 call v_c
 mov al,' '
 call v_c
 mov al,'('
 call v_c
 pop ax
 push cx
 mov cl,3
 call v_hexh
 pop cx
 mov al,')'
 call v_c
.n:
 add bx,2
 inc dh
 loop .p
 or si,si
 jnz .r
 VS v_none
.r:
 ret

; AX = BCD version (0x0300 -> 3.00)
v_bcd:
 push ax
 push cx
 mov al,ah
 call v_hexdig
 mov al,'.'
 call v_c
 pop cx
 pop ax
 push cx
 xor ah,ah
 mov cl,2
 call v_hex
 pop cx
 ret
v_hexdig:                         ; AL as two BCD digits, no leading zero above
 push ax
 push cx
 mov ah,al
 mov cl,4
 shr al,cl
 cmp ah,0x10
 jb .one
 add al,'0'
 call v_c
.one:
 mov al,ah
 and al,15
 add al,'0'
 call v_c
 pop cx
 pop ax
 ret

; state block string or "nicht gesetzt"
v_ls_or:
 mov es,[V_LSEG]
 cmp byte [es:si],0
 je .n
 call v_ls
 ret
.n:
 VS v_noset
 ret

s_commasp: db ', ',0
s_slash: db ' / ',0
s_slashsp: db ' / ',0
s_com: db 'COM',0
s_lpt: db 'LPT',0
; GEOS.INI sections and keys
s_sscreen: db 'screen 0',0
s_ssystem: db 'system',0
s_sui: db 'ui',0
s_skeyboard: db 'keyboard',0
s_smouse: db 'mouse',0
s_sprinter: db 'printer',0
s_swelcome: db 'welcome',0
s_sdiskswap: db 'diskswap',0
s_kdevice: db 'device',0
s_kdriver: db 'driver',0
s_kmemory: db 'memory',0
s_khandles: db 'handles',0
s_kfile: db 'file',0
s_ksize: db 'size',0
s_kgeneric: db 'generic',0
s_kspecific: db 'specific',0
s_kfontid: db 'fontid',0
s_kfontsize: db 'fontsize',0
s_kfontdrv: db 'font',0
s_kbackground: db 'background',0
s_kexec: db 'execonstartup',0
s_kprinters: db 'printers',0
s_knprint: db 'numberofprinters',0
s_ktask: db 'task driver',0
s_kversion: db 'versiontext',0
