# Test harness: runs GeoWorks 1.x Test code in an x86 emulator (Unicorn,
# 16-bit mode) with Python stand-ins for the GeoWorks kernel/UI calls, the
# BIOS interrupts and a fake DOSBox-like BIOS. Draws into PIL images.
import os, re, struct, sys
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_INTR, UC_HOOK_BLOCK, UC_HOOK_INSN
from unicorn.x86_const import *
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
CS, DG, SS = 0x1000, 0x3000, 0x3100
STOP = 0xFFF0
EGA = [(0,0,0),(0,0,170),(0,170,0),(0,170,170),(170,0,0),(170,0,170),(170,85,0),(170,170,170),
       (85,85,85),(85,85,255),(85,255,85),(85,255,255),(255,85,85),(255,85,255),(255,255,85),(255,255,255)]
GSTRING = 0x99
FONTDIR = '/usr/share/fonts/truetype/dejavu/'

def symbols():
    s = {}
    for line in open(os.path.join(HERE, 'g.map')):
        m = re.match(r'\s+([0-9A-F]+)\s+([0-9A-F]+)\s+(\S+)$', line)
        if m: s[m.group(3)] = int(m.group(1), 16)
    return s

def geos_text(b):
    return b.decode('mac_roman')

class Geos:
    def __init__(self, files_dir, vw=640, vh=460, ini=None):
        self.sym = symbols()
        raw = open(os.path.join(HERE, 'g.bin'), 'rb').read()
        ft = raw.rindex(b'G1META01')
        clen, mlen, rlen = struct.unpack('<HHH', raw[ft + 8:ft + 14])
        code = bytearray(raw[:clen])
        rel = raw[clen + mlen:clen + mlen + rlen]
        for i in range(0, len(rel), 4):
            typ, imp, site = struct.unpack('<BBH', rel[i:i + 4])
            c = site - 1
            assert code[c] == 0x9a, hex(c)
            ordn = code[c + 1:c + 3]
            intn = 0x80 if typ == 0 else 0x81
            code[c:c + 5] = bytes([0xcd, intn]) + ordn + b'\x90'
        self.mu = mu = Uc(UC_ARCH_X86, UC_MODE_16)
        mu.mem_map(0, 0x100000)
        mu.mem_write(CS * 16, bytes(code))
        mu.mem_write(CS * 16 + STOP, b'\xf4')
        self.next_seg = 0x5000
        self.handles = {}
        self.files = {}
        self.fdir = files_dir
        self.vw, self.vh = vw, vh
        self.canvas = Image.new('RGB', (vw, vh), (1, 2, 3))
        self.paper = Image.new('RGB', (620, 842 * 3), (255, 255, 255))
        self.area = (0, 0, 0); self.tcol = (0, 0, 0); self.lcol = (0, 0, 0)
        self.fsize = 10; self.style = 0; self.font = 0x1200
        self.ptx = self.pty = 0
        self.calls = {}
        self.dos = []
        self.ints = {}
        self.clip = None
        self.pc_msgs = []
        self.ini = ini if ini is not None else open(os.path.join(HERE, 'GEOS.INI'), 'rb').read()
        # fake BIOS
        bios = bytearray(0x10000)
        msg = b'IBM COMPATIBLE 486 BIOS COPYRIGHT The DOSBox Team.'
        bios[0xe061:0xe061 + len(msg)] = msg
        bios[0xfff5:0xfffd] = b'01/01/92'
        bios[0xfffe] = 0xfc
        mu.mem_write(0xf0000, bytes(bios))
        bda = bytearray(0x100)
        struct.pack_into('<HHHHHHH', bda, 0, 0x3f8, 0x2f8, 0, 0, 0x378, 0, 0)
        bda[0x75] = 1; bda[0x96] = 0x10
        mu.mem_write(0x400, bytes(bda))
        # INT 67h vector -> segment with "EMMXXXX0"
        mu.mem_write(0x67 * 4, struct.pack('<HH', 0, 0x9000))
        mu.mem_write(0x9000 * 16 + 10, b'EMMXXXX0')
        # environment of the "kernel" PSP
        mu.mem_write(0x8000 * 16 + 0x2c, struct.pack('<H', 0x8100))
        mu.mem_write(0x8100 * 16, b'PATH=C:\\DOS\0COMSPEC=Z:\\COMMAND.COM\0BLASTER=A220 I7 D1 H5 T6\0\0')
        # XMS entry: far return
        mu.mem_write(0x9100 * 16, b'\xcb')
        self.ticks = 0; self.blocks = 0
        mu.hook_add(UC_HOOK_INTR, self.intr)
        mu.hook_add(UC_HOOK_BLOCK, self.block)
        mu.hook_add(UC_HOOK_INSN, self.port_in, None, 1, 0, UC_X86_INS_IN)
        mu.hook_add(UC_HOOK_INSN, self.port_out, None, 1, 0, UC_X86_INS_OUT)
        self.cmos_idx = 0

    def block(self, mu, addr, size, ud):
        if addr == 0x91000:                           # XMS driver
            ah = self.reg(UC_X86_REG_AX) >> 8
            if ah == 0: self.setreg(UC_X86_REG_AX, 0x0300); self.setreg(UC_X86_REG_BX, 0x0350)
            elif ah == 8: self.setreg(UC_X86_REG_AX, 15360); self.setreg(UC_X86_REG_DX, 15360); self.setreg(UC_X86_REG_BX, 0)
        self.blocks += 1
        if self.blocks % 3000 == 0:
            self.ticks = (self.ticks + 1) & 0xffff
            mu.mem_write(0x46c, struct.pack('<H', self.ticks))

    def port_in(self, mu, port, size, ud):
        if port == 0x71: return {0x17: 0x00, 0x18: 0x3c}.get(self.cmos_idx, 0)
        if port == 0x61: return getattr(self, 'p61', 0x30)
        return 0
    def port_out(self, mu, port, size, value, ud):
        if port == 0x70: self.cmos_idx = value & 0x7f
        if port in (0x42, 0x43, 0x61):
            self.ports = getattr(self, 'ports', []) + [(port, value & 0xff)]
            if port == 0x61: self.p61 = value & 0xff

    def rd(self, seg, off, n): return bytes(self.mu.mem_read(seg * 16 + off, n))
    def wr(self, seg, off, b): self.mu.mem_write(seg * 16 + off, bytes(b))
    def w(self, seg, off): return struct.unpack('<H', self.rd(seg, off, 2))[0]
    def setw(self, seg, off, v): self.wr(seg, off, struct.pack('<H', v & 0xffff))
    def cstr(self, seg, off, enc='latin1'):
        b = self.rd(seg, off, 400); return b[:b.index(0)].decode(enc)
    def reg(self, r): return self.mu.reg_read(r)
    def setreg(self, r, v): self.mu.reg_write(r, v & 0xffff)
    def cf(self, on):
        f = self.mu.reg_read(UC_X86_REG_EFLAGS)
        self.mu.reg_write(UC_X86_REG_EFLAGS, (f | 1) if on else (f & ~1))

    def intr(self, mu, intno, ud):
        ip = self.reg(UC_X86_REG_IP)
        if intno in (0x80, 0x81):
            o = self.w(CS, ip)
            if intno == 0x80: self.hook(mu, o)
            else: self.uihook(mu, o)
            mu.reg_write(UC_X86_REG_IP, ip + 3)
            return
        self.ints[intno] = self.ints.get(intno, 0) + 1
        ax = self.reg(UC_X86_REG_AX); ah = ax >> 8
        if intno == 0x11: self.setreg(UC_X86_REG_AX, 0x4227 | 0x40)
        elif intno == 0x12: self.setreg(UC_X86_REG_AX, 640)
        elif intno == 0x15:
            if ah == 0x88: self.setreg(UC_X86_REG_AX, 0); self.cf(False)
        elif intno == 0x2f:
            if ax == 0x4300: self.setreg(UC_X86_REG_AX, 0x4380)
            elif ax == 0x4310: self.setreg(UC_X86_REG_ES, 0x9100); self.setreg(UC_X86_REG_BX, 0)
            elif ax == 0x1600: self.setreg(UC_X86_REG_AX, 0x1600)
        elif intno == 0x67:
            if ah == 0x40: self.setreg(UC_X86_REG_AX, 0)
            elif ah == 0x46: self.setreg(UC_X86_REG_AX, 0x0040)
            elif ah == 0x42: self.setreg(UC_X86_REG_AX, 0); self.setreg(UC_X86_REG_BX, 200); self.setreg(UC_X86_REG_DX, 256)
        elif intno == 0x10:
            if ah == 0x0f: self.setreg(UC_X86_REG_AX, 0x5012)
            elif ax == 0x4f00:
                es, di = self.reg(UC_X86_REG_ES), self.reg(UC_X86_REG_DI)
                assert self.rd(es, di, 4) == b'VBE2'
                info = bytearray(512); info[0:4] = b'VESA'
                struct.pack_into('<HHH', info, 4, 0x0200, 0x100 + di, es)
                struct.pack_into('<H', info, 0x12, 32)
                struct.pack_into('<HHHHHH', info, 0x16, 0x120 + di, es, 0x140 + di, es, 0, 0)
                info[0x100:0x100 + 26] = b'S3 Incorporated. Trio64\0\0\0'
                info[0x120:0x120 + 7] = b'DOSBox\0'
                info[0x140:0x140 + 7] = b'Trio64\0'
                self.wr(es, di, info)
                self.setreg(UC_X86_REG_AX, 0x004f)
            elif ax == 0x4f03: self.setreg(UC_X86_REG_AX, 0x004f); self.setreg(UC_X86_REG_BX, 0x4114)
            elif ax == 0x4f01:
                es, di = self.reg(UC_X86_REG_ES), self.reg(UC_X86_REG_DI)
                mi = bytearray(256); struct.pack_into('<HH', mi, 0x12, 800, 600); mi[0x19] = 16
                self.wr(es, di, mi); self.setreg(UC_X86_REG_AX, 0x004f)
        else:
            raise RuntimeError('unexpected INT %02xh' % intno)

    def uihook(self, mu, o):
        self.calls['ui%d' % o] = self.calls.get('ui%d' % o, 0) + 1
        if o == 135: self.setreg(UC_X86_REG_BX, 0x700)
        elif o == 127: self.clip_registered = True
        elif o in (213, 204): self.grabs = getattr(self, 'grabs', []) + [o]
        else: raise RuntimeError('unknown UI call %d' % o)

    def target(self):
        return self.reg(UC_X86_REG_DI) == GSTRING

    def draw(self):
        return ImageDraw.Draw(self.paper if self.target() else self.canvas)

    def off(self, x, y):
        if self.target(): return x + self.ptx, y + self.pty
        return x, y

    def hook(self, mu, o):
        self.calls[o] = self.calls.get(o, 0) + 1
        R = self.reg
        ax, bx, cx, dx, si, di, ds = (R(x) for x in (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_DS))
        sx = lambda v: v - 0x10000 if v & 0x8000 else v
        cf = False
        if o == 0:      # MemAlloc
            seg = self.next_seg; self.last = (seg, self.next_seg); self.next_seg += (ax + 15) // 16 + 1
            assert self.next_seg < 0x8000, 'out of test memory'
            self.wr(seg, 0, bytes(ax)); h = 0x100 + len(self.handles) * 16
            self.lasth = h
            self.handles[h] = seg
            self.setreg(UC_X86_REG_AX, seg); self.setreg(UC_X86_REG_BX, h)
        elif o == 2:
            if bx == getattr(self, 'lasth', None): self.next_seg = self.last[1]; self.lasth = None
        elif o in (4, 6, 24, 25, 28, 149, 398, 399, 446, 447, 212, 184): pass
        elif o == 411: self.setreg(UC_X86_REG_AX, 0x20)
        elif o == 5: self.setreg(UC_X86_REG_AX, self.handles.get(bx, 0))
        elif o == 95:   # DOS
            ah = ax >> 8; self.dos.append(ah)
            if ah == 0x30: self.setreg(UC_X86_REG_AX, 0x0005); self.setreg(UC_X86_REG_BX, 0xff00)
            elif ax == 0x3306: self.setreg(UC_X86_REG_AX, 0x3306); self.setreg(UC_X86_REG_BX, 0x0005)
            elif ah == 0x19: self.setreg(UC_X86_REG_AX, 0x1902)
            elif ah == 0x36:
                self.setreg(UC_X86_REG_AX, 64); self.setreg(UC_X86_REG_BX, 20000)
                self.setreg(UC_X86_REG_CX, 512); self.setreg(UC_X86_REG_DX, 65000)
            elif ah == 0x62: self.setreg(UC_X86_REG_BX, 0x8000)
            elif ah == 0x2a:
                self.setreg(UC_X86_REG_CX, 2026); self.setreg(UC_X86_REG_DX, 0x0b17); self.setreg(UC_X86_REG_AX, 0x2a01)
            elif ah == 0x2c:
                self.setreg(UC_X86_REG_CX, 0x0018); self.setreg(UC_X86_REG_DX, 0x2a00)
            else: raise RuntimeError('DOS %02x' % ah)
        elif o == 489:
            assert R(UC_X86_REG_ES) == DG and di in (0x40, 0x100)
            self.supers = getattr(self, 'supers', []) + [(di, ax)]
        elif o == 76: self.cwd = ''
        elif o == 51: self.cwd = self.cstr(ds, dx) + '\\'
        elif o == 73: self.dirs = getattr(self, 'dirs', []) + [getattr(self, 'cwd', '')]
        elif o == 74: self.cwd = self.dirs.pop()
        elif o == 75: self.wr(ds, si, b'\\GEOWORKS\0'); self.setreg(UC_X86_REG_BX, 1)
        elif o == 211: self.setreg(UC_X86_REG_DI, 0x77)
        elif o == 65:   # FileEnum: parameter block on the stack, removed
            ss, sp = R(UC_X86_REG_SS), R(UC_X86_REG_SP)
            blk = self.rd(ss, sp, 0x30)
            boff, bseg = struct.unpack_from('<HH', blk, 0x12)
            maxn, = struct.unpack_from('<H', blk, 0x16)
            pre = getattr(self, 'cwd', '').replace('\\', '_')
            names = sorted(f[len(pre):] for f in os.listdir(self.fdir) if pre and f.startswith(pre))
            ents = [(0x10, 'TTF')] + [(0x20, n) for n in names]
            ents = ents[:maxn]
            for i, (attr, n) in enumerate(ents):
                e = bytearray(22); e[0] = attr; e[9:9 + len(n)] = n.encode()
                self.wr(bseg, boff + i * 22, e)
            self.setreg(UC_X86_REG_BX, len(ents)); self.setreg(UC_X86_REG_DX, 0)
            self.mu.reg_write(UC_X86_REG_SP, sp + 0x30)
            self.enums = getattr(self, 'enums', 0) + 1
        elif o == 440:
            self.setreg(UC_X86_REG_AX, 0); self.setreg(UC_X86_REG_BX, 0)
            self.setreg(UC_X86_REG_CX, self.vw - 1); self.setreg(UC_X86_REG_DX, self.vh - 1)
        elif o == 310:
            if self.target(): self.ptx += sx(dx); self.pty += sx(bx)
            else: assert dx == 0 and bx == 0
        elif o == 150: self.setreg(UC_X86_REG_BX, 0x3333)
        elif o == 169:
            self.pc_msgs.append(ax)
            if ax == 0x2063: self.setreg(UC_X86_REG_BP, 0x77)
            if ax == 0x6016:
                self.setreg(UC_X86_REG_AX, 36); self.setreg(UC_X86_REG_CX, 36); self.setreg(UC_X86_REG_DX, 36); self.setreg(UC_X86_REG_BP, 36)
            elif ax == 0x6012:
                self.setreg(UC_X86_REG_CX, 595); self.setreg(UC_X86_REG_DX, 842)
            elif ax == 0x6005: self.range = (cx, dx)
            elif ax == 0x6009: self.docsize = (cx, dx)
            elif ax == 0x6013 or ax == 0x777: self.docname = self.cstr(cx, dx)
        elif o in (52, 53):
            name = self.cstr(ds, dx)
            full = getattr(self, 'cwd', '') + name
            if o == 52 and full.upper() == 'GEOS.INI':
                data = bytearray(self.ini)
            elif o == 52 and full.upper() == 'KERNEL.EXE':
                data = bytearray(72937)
            else:
                path = os.path.join(self.fdir, full.replace('\\', '_'))
                if o == 52:
                    if not os.path.exists(path): cf = True; self.setreg(UC_X86_REG_AX, 2)
                    else: data = bytearray(open(path, 'rb').read())
                else:
                    data = bytearray(); assert ax == 0x00c1, hex(ax)
            if not cf:
                h = 0x40 + len(self.files) * 2
                while h in self.files: h += 2
                self.files[h] = {'data': data, 'pos': 0, 'name': full, 'write': o == 53,
                                 'path': os.path.join(self.fdir, full.replace('\\', '_'))}
                self.setreg(UC_X86_REG_AX, h)
        elif o == 54:
            f = self.files.pop(bx)
            if f['write']: open(f['path'], 'wb').write(f['data'])
        elif o == 58:
            f = self.files[bx]; d = f['data'][f['pos']:f['pos'] + cx]
            self.wr(ds, dx, d); f['pos'] += len(d)
            if len(d) < cx: cf = True; self.setreg(UC_X86_REG_CX, len(d))
        elif o == 59:
            f = self.files[bx]; assert f['write']; d = self.rd(ds, dx, cx)
            f['data'][f['pos']:f['pos'] + cx] = d; f['pos'] += cx
        elif o == 60:
            f = self.files[bx]; off = (cx << 16 | dx)
            base = {0: 0, 1: f['pos'], 2: len(f['data'])}[ax & 0xff]
            f['pos'] = max(0, base + off)
            self.setreg(UC_X86_REG_AX, f['pos'] & 0xffff); self.setreg(UC_X86_REG_DX, f['pos'] >> 16)
        elif o == 355:
            self.area = (ax & 0xff, bx & 0xff, bx >> 8) if (ax >> 8) == 0x80 else EGA[ax & 15]
        elif o == 359: self.tcol = EGA[ax & 15]
        elif o == 346: self.lcol = EGA[ax & 15]
        elif o == 356: self.mask = ax & 0xff
        elif o == 366: self.font, self.fsize = cx, dx
        elif o == 362: self.style = (self.style & ~(ax >> 8)) | (ax & 0xff)
        elif o == 336:
            x0, y0 = self.off(sx(ax), sx(bx)); x1, y1 = self.off(sx(cx), sx(dx))
            if x1 > x0 and y1 > y0:
                self.draw().rectangle((x0, y0, x1 - 1, y1 - 1), fill=self.area)
        elif o == 316:
            x0, y0 = self.off(sx(ax), sx(bx)); x1, y1 = self.off(sx(cx), sx(dx))
            self.draw().rectangle((x0, y0, x1, y1), outline=self.lcol)
        elif o == 324:
            x0, y0 = self.off(sx(ax), sx(bx)); x1, y1 = self.off(sx(cx), sx(dx))
            self.draw().ellipse((x0, y0, x1, y1), outline=self.lcol)
        elif o == 330:
            t = self.rd(ds, si, 300); t = geos_text(t[:t.index(0)])
            bold = bool(self.style & 0x20)
            name = 'DejaVuSans%s.ttf' % ('-Bold' if bold else '')
            if self.font == 0x1a00: name = 'DejaVuSansMono%s.ttf' % ('-Bold' if bold else '')
            if self.font == 0x1000: name = 'DejaVuSerif%s.ttf' % ('-Bold' if bold else '')
            f = ImageFont.truetype(FONTDIR + name, self.fsize + 1)
            x, y = self.off(sx(ax), sx(bx))
            self.draw().text((x, y - 1), t, fill=self.tcol, font=f)
            self.texts = getattr(self, 'texts', []) + [t]
        elif o == 326:
            wd, ht, comp, typ = struct.unpack('<HHBB', self.rd(ds, si, 6))
            x, y = self.off(sx(ax), sx(bx))
            self.bitmaps = getattr(self, 'bitmaps', []) + [(typ, comp, wd, ht)]
            if typ == 3 and comp == 0:
                px = self.rd(ds, si + 6, wd * 3)
                for i in range(wd):
                    if 0 <= x + i < self.vw and 0 <= y < self.vh:
                        self.canvas.putpixel((x + i, y), tuple(px[i * 3:i * 3 + 3]))
            else:
                self.draw().rectangle((x, y, x + wd - 1, y + ht - 1), outline=(255, 0, 255))
        else:
            raise RuntimeError('unknown kernel call %d' % o)
        self.cf(cf)

    def call(self, name, far=False, **regs):
        mu = self.mu
        sp = 0xFF00
        if far:
            sp -= 4; self.wr(SS, sp, struct.pack('<HH', STOP, CS))
        else:
            sp -= 2; self.wr(SS, sp, struct.pack('<H', STOP))
        mu.reg_write(UC_X86_REG_SS, SS); mu.reg_write(UC_X86_REG_SP, sp)
        mu.reg_write(UC_X86_REG_DS, DG); mu.reg_write(UC_X86_REG_ES, DG)
        mu.reg_write(UC_X86_REG_CS, CS)
        for k, v in regs.items(): mu.reg_write(getattr(sys.modules['unicorn.x86_const'], 'UC_X86_REG_' + k.upper()), v & 0xffff)
        mu.emu_start(CS * 16 + self.sym[name], CS * 16 + STOP, count=400_000_000)
        assert mu.reg_read(UC_X86_REG_SP) == 0xFF00, 'stack not balanced after %s: %x' % (name, mu.reg_read(UC_X86_REG_SP))
        assert mu.reg_read(UC_X86_REG_DS) == DG, 'DS changed by %s' % name

    def v(self, off): return self.w(DG, off)
    def setv(self, off, val): self.setw(DG, off, val)
