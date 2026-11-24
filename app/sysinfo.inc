; SPDX-License-Identifier: MIT
; sysinfo.inc - machine detection for GeoWorks 1.x Test.
; Results go into the state block (H_* offsets). Only reading BIOS/DOS
; queries are used; nothing is changed.

gather:
 cld
 call cpu_detect
 call fpu_detect
 call cpu_speed
 call bios_detect
 call mem_detect
 call dos_detect
 call video_detect
 call env_detect
 call geos_detect
 ret

; ----------------------------------------------------------------------
; processor
; ----------------------------------------------------------------------
cpu_detect:
 mov es,[V_LSEG]
 pushf
 pop cx
 mov ax,cx
 and ax,0x0fff
 push ax
 popf
 pushf
 pop ax
 push cx
 popf
 and ax,0xf000
 cmp ax,0xf000
 jne .not86
 ; 8086/8088 or 80186/80188: from the 186 on the shift count is masked
 mov ax,0xffff
 mov cl,33
 shl ax,cl
 mov word [es:H_CPU],0
 or ax,ax
 jz .r
 mov word [es:H_CPU],1
 ret
.not86:
 ; 286: flag bits 12..14 stay 0 in real mode
 pushf
 pop cx
 mov ax,cx
 or ax,0x7000
 push ax
 popf
 pushf
 pop ax
 push cx
 popf
 test ax,0x7000
 jnz .is386
 mov word [es:H_CPU],2
.r:
 ret
.is386:
 mov word [es:H_CPU],3
CPU 686
 cli
 pushfd
 pop eax
 mov ecx,eax
 xor eax,0x40000                  ; AC flag: only 486 and later
 push eax
 popfd
 pushfd
 pop eax
 push ecx
 popfd
 xor eax,ecx
 test eax,0x40000
 jz .done
 mov word [es:H_CPU],4
 mov eax,ecx
 xor eax,0x200000                 ; ID flag: CPUID available
 push eax
 popfd
 pushfd
 pop eax
 push ecx
 popfd
 xor eax,ecx
 test eax,0x200000
 jz .done
 mov word [es:H_CPUID],1
 xor eax,eax
 cpuid
 mov [es:H_CPUIDMX],ax
 mov [es:H_VENDOR],ebx
 mov [es:H_VENDOR+4],edx
 mov [es:H_VENDOR+8],ecx
 mov byte [es:H_VENDOR+12],0
 or eax,eax
 jz .done
 mov eax,1
 cpuid
 mov [es:L_SIG],eax
 mov [es:H_FEAT],edx
 mov eax,0x80000000
 cpuid
 cmp eax,0x80000004
 jb .done
 cmp eax,0x800000ff
 ja .done
 mov di,H_BRAND
 mov esi,0x80000002
.brand:
 mov eax,esi
 cpuid
 mov [es:di],eax
 mov [es:di+4],ebx
 mov [es:di+8],ecx
 mov [es:di+12],edx
 add di,16
 inc esi
 cmp esi,0x80000004
 jbe .brand
 mov byte [es:di],0
.done:
 sti
CPU 8086
 ; family / model / stepping from the signature
 mov ax,[es:L_SIG]
 mov bx,ax
 and bx,15
 mov [es:H_STEP],bx
 mov bx,ax
 mov cl,4
 shr bx,cl
 and bx,15
 mov [es:H_MOD],bx
 mov bl,ah
 and bx,15
 mov [es:H_FAM],bx
 cmp bx,15
 jne .nf
 mov al,[es:L_SIG+2]              ; extended family: bits 20..27
 mov ah,[es:L_SIG+3]
 shr ax,cl
 and ax,0xff
 add [es:H_FAM],ax
.nf:
 cmp bx,6
 jb .nm
 mov al,[es:L_SIG+2]              ; extended model: bits 16..19
 and ax,15
 shl ax,cl
 add [es:H_MOD],ax
.nm:
 ret

fpu_detect:
 mov es,[V_LSEG]
 mov word [es:H_FPU],0
 fninit
 mov cx,20
.w:
 loop .w
 mov word [es:L_TMPW],0x5a5a
 fnstsw [es:L_TMPW]
 mov cx,20
.w2:
 loop .w2
 cmp byte [es:L_TMPW],0
 jne .r
 fnstcw [es:L_TMPW]
 mov cx,20
.w3:
 loop .w3
 mov ax,[es:L_TMPW]
 and ax,0x103f
 cmp ax,0x003f
 jne .r
 mov word [es:H_FPU],1
.r:
 ret

; ----------------------------------------------------------------------
; speed: LOOP iterations per second, counted against the BIOS clock
; (0040:006C, 18.2 ticks per second) over 4 ticks
; ----------------------------------------------------------------------
cpu_speed:
 mov es,[V_LSEG]
 mov word [es:H_SPD],0
 call tick_sync
 jc .r
 mov [es:L_TICK],bx
 xor si,si                        ; blocks of 500 loops
.blk:
 mov cx,500
.l:
 loop .l
 inc si
 jz .r                            ; far too fast: give up
 push es
 mov ax,0x40
 mov es,ax
 mov ax,[es:0x6c]
 pop es
 sub ax,[es:L_TICK]
 cmp ax,4
 jb .blk
 ; loops/s = blocks * 500 * 18.2065 / ticks(4) = blocks * 2275.8
 mov ax,si
 mov cx,2276
 mul cx
 mov [es:H_LOOPLO],ax
 mov [es:H_LOOPHI],dx
 ; index: IBM PC/XT (8088, 4.77 MHz) makes about 280750 loops/s
 mov cx,28075
 cmp dx,cx
 jae .r
 div cx                           ; index * 10
 mov [es:H_SPDIX],ax
 mov word [es:H_SPD],1
.r:
 ret

; wait for the next BIOS tick -> BX = tick count; CF = clock does not run
tick_sync:
 push es
 push dx
 mov ax,0x40
 mov es,ax
 mov bx,[es:0x6c]
 mov dx,40
.o:
 xor cx,cx
.w:
 cmp bx,[es:0x6c]
 jne .edge
 loop .w
 dec dx
 jnz .o
 pop dx
 pop es
 stc
 ret
.edge:
 mov bx,[es:0x6c]
 pop dx
 pop es
 clc
 ret
; ticks since BX -> AX
tick_since:
 push es
 mov ax,0x40
 mov es,ax
 mov ax,[es:0x6c]
 sub ax,bx
 pop es
 ret

; ----------------------------------------------------------------------
; BIOS: date, model byte, equipment, ports, emulator signatures
; ----------------------------------------------------------------------
bios_detect:
 mov es,[V_LSEG]
 push ds
 mov ax,0xf000
 mov ds,ax
 mov si,0xfff5
 mov di,H_BDATE
 mov cx,8
.d:
 lodsb
 cmp al,' '
 jb .dq
 cmp al,0x7e
 jbe .dok
.dq:
 mov al,'?'
.dok:
 stosb
 loop .d
 mov byte [es:di],0
 mov al,[0xfffe]
 xor ah,ah
 mov [es:H_MODB],ax
 pop ds
 int 0x11
 mov [es:H_EQ],ax
 ; BIOS data area: ports, hard disks, keyboard
 push ds
 mov ax,0x40
 mov ds,ax
 mov si,0
 mov di,H_COM
 mov cx,7
 rep movsw                        ; COM1-4, LPT1-3
 mov al,[0x75]
 xor ah,ah
 mov [es:H_HDD],ax
 mov al,[0x96]
 mov [es:H_KBD],ax
 pop ds
 ; emulator / BIOS maker: first in the system BIOS, then the video BIOS
 mov word [es:H_EMU],0
 mov ax,0xf000
 mov cx,0xfff0
 call sig_scan
 cmp word [es:H_EMU],1
 je .r
 mov ax,0xc000
 mov cx,0x7ff0
 call sig_scan
.r:
 ret

; search segment AX, offsets 0..CX for the signatures (case-insensitive)
sig_scan:
 mov [es:L_TMPW2],ax
 mov [es:L_TMPW3],cx
 mov bx,sigs
.sig:
 cmp byte [cs:bx],0xff
 jne .try
 ret
.try:
 ; a vendor signature is not wanted once something was found
 cmp byte [cs:bx],0
 jne .want
 cmp word [es:H_EMU],0
 je .want
 jmp .skip
.want:
 push ds
 mov ds,[es:L_TMPW2]
 mov dl,[cs:bx+1]                 ; first character
 xor si,si
 mov cx,[es:L_TMPW3]
.pos:
 mov al,[si]
 or al,0x20
 cmp al,dl
 je .cmp
.pn:
 inc si
 loop .pos
 pop ds
 jmp .skip
.cmp:
 push si
 push bx
 inc bx
.c2:
 mov ah,[cs:bx]
 or ah,ah
 jz .hit
 mov al,[si]
 or al,0x20
 cmp al,ah
 jne .miss
 inc si
 inc bx
 jmp .c2
.miss:
 pop bx
 pop si
 jmp .pn
.hit:
 pop bx
 pop si
 ; printable run around the hit -> H_BIOSS
 mov dx,si
 mov cx,40
.back:
 or si,si
 jz .bs
 mov al,[si-1]
 cmp al,' '
 jb .bs
 cmp al,0x7e
 ja .bs
 dec si
 loop .back
.bs:
 mov di,H_BIOSS
 mov cx,63
.fw:
 mov al,[si]
 cmp al,' '
 jb .fe
 cmp al,0x7e
 ja .fe
 mov [es:di],al
 inc si
 inc di
 loop .fw
.fe:
 mov byte [es:di],0
 pop ds
 ; label: after the pattern
 mov al,[cs:bx]
 inc bx
.sk:
 cmp byte [cs:bx],0
 je .lb
 inc bx
 jmp .sk
.lb:
 inc bx
 mov [es:H_EMUN],bx
 mov word [es:H_EMU],2
 or al,al
 jz .vend
 mov word [es:H_EMU],1
 ret                              ; an emulator: done
.vend:
 ret
.skip:
 inc bx                           ; flag
.s1:
 cmp byte [cs:bx],0
 je .s2
 inc bx
 jmp .s1
.s2:
 inc bx
.s3:
 cmp byte [cs:bx],0
 je .s4
 inc bx
 jmp .s3
.s4:
 inc bx
 jmp .sig

; flag (1 emulator, 0 BIOS maker), pattern in lower case, 0, label, 0
sigs:
 db 1,'dosbox-x',0,'DOSBox-X',0
 db 1,'basebox',0,'Basebox (DOSBox-Variante)',0
 db 1,'dosbox staging',0,'DOSBox Staging',0
 db 1,'dosbox',0,'DOSBox',0
 db 1,'86box',0,'86Box',0
 db 1,'pcem',0,'PCem',0
 db 1,'qemu',0,'QEMU',0
 db 1,'bochs',0,'Bochs',0
 db 1,'virtualbox',0,'VirtualBox',0
 db 1,'vbox',0,'VirtualBox',0
 db 1,'vmware',0,'VMware',0
 db 1,'seabios',0,'SeaBIOS (QEMU/KVM)',0
 db 1,'pcjs',0,'PCjs',0
 db 1,'martypc',0,'MartyPC',0
 db 0,'award',0,'Award-BIOS',0
 db 0,'phoenix',0,'Phoenix-BIOS',0
 db 0,'american megatrends',0,'AMI-BIOS',0
 db 0,'compaq',0,'Compaq-BIOS',0
 db 0,'mr bios',0,'MR BIOS',0
 db 0,'quadtel',0,'Quadtel-BIOS',0
 db 0,'ibm',0,'IBM-BIOS',0
 db 0xff

; ----------------------------------------------------------------------
; memory: INT 12h, CMOS, INT 15h, XMS, EMS
; ----------------------------------------------------------------------
mem_detect:
 mov es,[V_LSEG]
 int 0x12
 mov [es:H_CONV],ax
 mov word [es:H_CMOS],0
 mov word [es:H_I15],0xffff
 cmp word [es:H_CPU],2
 jb .xms
 cli
 mov al,0x18
 out 0x70,al
 jmp short $+2
 in al,0x71
 mov ah,al
 mov al,0x17
 out 0x70,al
 jmp short $+2
 in al,0x71
 mov bx,ax
 mov al,0x0d
 out 0x70,al
 sti
 mov [es:H_CMOS],bx
 mov ah,0x88
 clc
 int 0x15
 jc .xms
 mov [es:H_I15],ax
.xms:
 mov es,[V_LSEG]
 mov word [es:H_XMS],0
 mov ax,0x4300
 int 0x2f
 cmp al,0x80
 jne .ems
 push es
 mov ax,0x4310
 int 0x2f
 mov ax,es
 pop es
 mov [es:L_XMSP],bx
 mov [es:L_XMSP+2],ax
 mov word [es:H_XMS],1
 xor ah,ah
 call far [es:L_XMSP]             ; version
 mov es,[V_LSEG]
 mov [es:H_XMSV],ax
 mov [es:H_XMSR],bx
 mov ah,8
 xor bl,bl
 call far [es:L_XMSP]             ; free memory
 mov es,[V_LSEG]
 mov [es:H_XMSL],ax
 mov [es:H_XMSF],dx
.ems:
 mov es,[V_LSEG]
 mov word [es:H_EMS],0
 push ds
 xor ax,ax
 mov ds,ax
 mov ax,[0x67*4+2]                ; segment of INT 67h
 mov ds,ax
 mov si,0x0a
 mov bx,s_emm
 mov cx,8
.e:
 mov al,[si]
 cmp al,[cs:bx]
 jne .noems
 inc si
 inc bx
 loop .e
 pop ds
 mov ah,0x40
 int 0x67
 or ah,ah
 jnz .r
 mov ah,0x46
 int 0x67
 or ah,ah
 jnz .r
 mov es,[V_LSEG]
 xor ah,ah
 mov [es:H_EMSV],ax
 mov ah,0x42
 int 0x67
 or ah,ah
 jnz .r
 mov es,[V_LSEG]
 mov [es:H_EMSF],bx
 mov [es:H_EMST],dx
 mov word [es:H_EMS],1
.r:
 ret
.noems:
 pop ds
 ret
s_emm: db 'EMMXXXX0'

; ----------------------------------------------------------------------
; DOS through kernel 95 (passes a DOS function on, as DCLOCK24 does)
; ----------------------------------------------------------------------
dos_call:
 push ds
 push es
 push si
 push di
 push bp
 KCALL 95
 pop bp
 pop di
 pop si
 pop es
 pop ds
 ret

dos_detect:
 mov ax,0x3000
 call dos_call
 mov es,[V_LSEG]
 mov [es:H_DOSV],ax
 mov al,bh
 xor ah,ah
 mov [es:H_DOSOEM],ax
 mov word [es:H_TRUEV],0
 cmp byte [es:H_DOSV],5
 jb .drv
 mov ax,0x3306
 xor bx,bx
 call dos_call
 mov es,[V_LSEG]
 cmp al,0xff
 je .drv
 mov [es:H_TRUEV],bx
.drv:
 mov ah,0x19
 call dos_call
 mov es,[V_LSEG]
 xor ah,ah
 mov [es:H_DRV],ax
 ; free space of the current drive in KB
 mov word [es:H_DOK],0
 mov ah,0x36
 xor dl,dl
 call dos_call
 cmp ax,0xffff
 je .win
 push dx                          ; total clusters
 push bx                          ; free clusters
 mul cx                           ; bytes per cluster
 mov cx,10
.k:
 shr dx,1                         ; / 1024
 rcr ax,1
 dec cx
 jnz .k
 mov cx,ax                        ; KB per cluster (>= 1 for sectors >= 512)
 or cx,cx
 jnz .k1
 inc cx
.k1:
 pop ax
 mul cx
 mov es,[V_LSEG]
 mov [es:H_DFREE],ax
 mov [es:H_DFREE+2],dx
 pop ax
 mul cx
 mov [es:H_DTOT],ax
 mov [es:H_DTOT+2],dx
 mov word [es:H_DOK],1
.win:
 ; Windows (INT 2Fh 1600h)
 mov ax,0x1600
 int 0x2f
 mov es,[V_LSEG]
 mov [es:H_WIN],ax
 ret

; date and time (refreshed for every painting)
get_dt:
 mov ah,0x2a
 call dos_call
 mov es,[V_LSEG]
 mov [es:H_YEAR],cx
 mov [es:H_MD],dx
 xor ah,ah
 mov [es:H_WD],ax
 mov ah,0x2c
 call dos_call
 mov es,[V_LSEG]
 mov [es:H_HM],cx
 mov [es:H_SEC],dx
 ret

; ----------------------------------------------------------------------
; video: BIOS mode, VESA BIOS extension
; ----------------------------------------------------------------------
video_detect:
 push bp
 push si
 push di
 mov ah,0x0f
 int 0x10
 mov es,[V_LSEG]
 mov bl,ah
 xor ah,ah
 and al,0x7f
 mov [es:H_VMODE],ax
 mov bh,0
 mov [es:H_VCOLS],bx
 ; VBE controller information
 mov word [es:H_VBE],0
 mov di,L_TMP
 mov word [es:di],'VB'
 mov word [es:di+2],'E2'
 mov ax,0x4f00
 int 0x10
 mov es,[V_LSEG]
 cmp ax,0x004f
 je .v1
 jmp .novbe
.v1:
 cmp word [es:L_TMP],'VE'
 je .v2
 jmp .novbe
.v2:
 mov word [es:H_VBE],1
 mov ax,[es:L_TMP+4]
 mov [es:H_VBEV],ax
 mov ax,[es:L_TMP+0x12]
 mov [es:H_VBEMEM],ax
 mov si,[es:L_TMP+6]
 mov ax,[es:L_TMP+8]
 mov di,H_VBEOEM
 mov cx,47
 call copy_far
 mov byte [es:H_VBEVEN],0
 mov byte [es:H_VBEPRD],0
 cmp word [es:H_VBEV],0x0200
 jb .mode
 mov si,[es:L_TMP+0x16]
 mov ax,[es:L_TMP+0x18]
 mov di,H_VBEVEN
 mov cx,39
 call copy_far
 mov si,[es:L_TMP+0x1a]
 mov ax,[es:L_TMP+0x1c]
 mov di,H_VBEPRD
 mov cx,39
 call copy_far
.mode:
 mov ax,0x4f03
 int 0x10
 mov es,[V_LSEG]
 cmp ax,0x004f
 jne .novbe
 and bx,0x3fff
 mov [es:H_VMODEV],bx
 mov cx,bx
 mov di,L_MI
 mov ax,0x4f01
 int 0x10
 mov es,[V_LSEG]
 cmp ax,0x004f
 jne .table
 mov ax,[es:L_MI+0x12]
 mov [es:H_XRES],ax
 mov ax,[es:L_MI+0x14]
 mov [es:H_YRES],ax
 mov al,[es:L_MI+0x19]
 xor ah,ah
 mov [es:H_BPP],ax
 mov word [es:H_MIOK],1
 jmp .r
.novbe:
 mov es,[V_LSEG]
 mov ax,[es:H_VMODE]
 mov [es:H_VMODEV],ax
.table:
 ; standard BIOS modes
 mov es,[V_LSEG]
 mov ax,[es:H_VMODEV]
 mov si,vmodes
.t:
 cmp word [cs:si],0xffff
 je .r
 cmp ax,[cs:si]
 je .tm
 add si,8
 jmp .t
.tm:
 mov ax,[cs:si+2]
 mov [es:H_XRES],ax
 mov ax,[cs:si+4]
 mov [es:H_YRES],ax
 mov ax,[cs:si+6]
 mov [es:H_BPP],ax
 mov word [es:H_MIOK],1
.r:
 pop di
 pop si
 pop bp
 ret
vmodes:
 dw 0x04,320,200,2, 0x06,640,200,1, 0x0d,320,200,4, 0x0e,640,200,4
 dw 0x0f,640,350,1, 0x10,640,350,4, 0x11,640,480,1, 0x12,640,480,4
 dw 0x13,320,200,8, 0x6a,800,600,4, 0x102,800,600,4, 0x100,640,400,8
 dw 0x101,640,480,8, 0x103,800,600,8, 0x105,1024,768,8, 0xffff

; far string AX:SI -> ES:DI, at most CX characters, printable only
copy_far:
 push ds
 or ax,ax
 jz .e
 mov ds,ax
.l:
 mov al,[si]
 or al,al
 jz .e
 cmp al,' '
 jb .e
 cmp al,0x7e
 ja .q
 jmp .s
.q:
 mov al,'?'
.s:
 mov [es:di],al
 inc si
 inc di
 loop .l
.e:
 mov byte [es:di],0
 pop ds
 ret

; ----------------------------------------------------------------------
; DOS environment of the GEOS kernel: COMSPEC, BLASTER
; ----------------------------------------------------------------------
env_detect:
 mov ah,0x62
 call dos_call
 mov es,[V_LSEG]
 mov [es:H_PSP],bx
 or bx,bx
 jz .r
 push ds
 mov ds,bx
 mov ax,[0x2c]                    ; environment segment
 pop ds
 or ax,ax
 jz .r
 mov [es:L_TMPW],ax
 mov bx,s_comspec
 mov di,H_COMSPEC
 call env_get
 mov bx,s_blaster
 mov di,H_BLASTER
 call env_get
.r:
 ret

; CS:BX = "NAME=", ES:DI = destination (63 characters)
env_get:
 mov byte [es:di],0
 push ds
 mov ds,[es:L_TMPW]
 xor si,si
.var:
 cmp byte [si],0
 je .none
 push si
 push bx
.c:
 mov al,[cs:bx]
 or al,al
 jz .found
 cmp al,[si]
 jne .no
 inc si
 inc bx
 jmp .c
.no:
 pop bx
 pop si
.sk:
 lodsb
 or al,al
 jnz .sk
 cmp si,0x7f00
 jb .var
 jmp .none
.found:
 pop bx
 pop ax
 mov cx,63
.cp:
 lodsb
 or al,al
 jz .ce
 cmp al,' '
 jb .ce
 cmp al,0x7e
 ja .ce
 mov [es:di],al
 inc di
 loop .cp
.ce:
 mov byte [es:di],0
.none:
 pop ds
 ret
s_comspec: db 'COMSPEC=',0
s_blaster: db 'BLASTER=',0

; ----------------------------------------------------------------------
; GEOS: directory, size of KERNEL.EXE
; ----------------------------------------------------------------------
geos_detect:
 KCALL 73                         ; FilePushDir
 mov ax,0
 KCALL 76                         ; FileSetStandardPath: GEOWORKS
 jc .pop
 push ds
 mov ds,[V_LSEG]
 mov si,H_GPATH
 mov byte [si],0
 mov cx,79
 KCALL 75                         ; FileGetCurrentPath DS:SI
 pop ds
 mov es,[V_LSEG]
 mov word [es:H_KOK],0
 push ds
 push cs
 pop ds
 mov dx,s_kernel
 mov al,0x40                      ; read, deny none
 KCALL 52                         ; FileOpen
 pop ds
 jc .pop
 mov bx,ax
 push bx
 mov al,2
 xor cx,cx
 xor dx,dx
 KCALL 60                         ; FilePos: end
 mov es,[V_LSEG]
 mov [es:H_KSZLO],ax
 mov [es:H_KSZHI],dx
 mov word [es:H_KOK],1
 pop bx
 mov al,0
 KCALL 54                         ; FileClose
.pop:
 KCALL 74                         ; FilePopDir
 ret
s_kernel: db 'KERNEL.EXE',0

; true colour screen driver? GEOS.INI [screen 0] device names 64K, 32K,
; 16M or TrueColor
detect_tc:
 mov word [V_TC],0
 call v_reset
 mov si,s_sscreen
 mov bx,s_kdevice
 call ini_get
 jc .r
 mov es,[V_LSEG]
 mov si,L_VAL
.l:
 mov ax,[es:si]
 or al,al
 jz .r
 cmp ax,'64'
 je .k
 cmp ax,'32'
 je .k
 cmp ax,'16'
 jne .n
 mov al,[es:si+2]
 and al,0xdf
 cmp al,'M'
 je .tc
 jmp .n
.k:
 mov al,[es:si+2]
 and al,0xdf
 cmp al,'K'
 je .tc
.n:
 mov ax,[es:si]
 and ax,0xdfdf
 cmp ax,'TR'
 jne .n2
 mov al,[es:si+2]
 and al,0xdf
 cmp al,'U'
 je .tc
.n2:
 inc si
 jmp .l
.tc:
 mov word [V_TC],1
.r:
 ret
