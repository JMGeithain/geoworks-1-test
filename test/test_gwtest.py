# Emulator test of GeoWorks 1.x Test: start, all pages, measurement,
# saving, clipboard, printing. Screenshots go to test/out/.
import os, shutil, struct
from emu import Geos, DG, GSTRING

V = dict(WIN=0xDE, INIT=0xE0, LSEG=0xE2, PAGE=0xE6, VW=0xE8, VH=0xEA, TC=0xF0, SCR=0xF2, MSG=0xFA, NROW=0xFE)
D = '/tmp/gwtest'
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'out')
shutil.rmtree(D, ignore_errors=True); os.makedirs(D); os.makedirs(OUT, exist_ok=True)
ok = 0
L_NFONT, L_FONTS, L_INSTR, L_SNDRUN, L_SNDP = 1082, 34348, 1096, 1090, 1088

# synthetic font files (header as in GeoWorks 1.2): GEOWORKS\FONT\*.FNT
def fnt(fid, maker, name, sizes=(), outline=False, extra=()):
    ent = b''.join(struct.pack('<BBHHI', st, 0, sz, 100, 0) for st, sz in list(sizes) + list(extra))
    pt = 0x23; pe = pt + len(ent); oe = pe + (0x68 if outline else 0)
    h = bytearray(b'BSWF') + struct.pack('<HHHHB', 0x100, 0x23 + len(ent), fid, maker, 0x80)
    h += name.encode().ljust(20, b'\0') + struct.pack('<HHHH', pt, pe, pe, oe)
    h += ent
    return bytes(h) + bytes(400)
fonts = {'BERKELEY.FNT': fnt(0x0202, 0, 'Berkeley', [(0, 14), (0, 9), (0, 10), (0, 12), (0, 18), (0, 12)], extra=[(0x20, 11)]),
         'SANS.FNT': fnt(0x1200, 0x1000, 'URW Sans', [(0, 10), (0, 12)], outline=True),
         'LED.FNT': fnt(0x0600, 0, 'LED', [(0, 30), (0, 40), (0, 18)]),
         'PR_10CPI.FNT': fnt(0xfa00, 0xf000, 'Printer 10cpi', [(0, 12)]),
         'DINGBATS.FNT': fnt(0xd09a, 0x1000, 'dingbats-Thin', outline=True),
         'KAPUTT.FNT': b'MZ' + bytes(300), 'LIESMICH.TXT': b'BSWF' + bytes(100)}
for n, d in fonts.items():
    open(os.path.join(D, 'FONT_' + n), 'wb').write(d)
def font_list(g):
    seg = g.v(V['LSEG']); n = g.w(seg, L_NFONT); out = []
    for i in range(n):
        r = g.rd(seg, L_FONTS + i * 48, 48)
        out.append((r[0:21].split(b'\0')[0].decode(), r[21:34].split(b'\0')[0].decode(),
                    struct.unpack_from('<H', r, 34)[0], r[38], list(r[40:40 + r[39]])))
    return out
def check(cond, what):
    global ok
    if not cond: raise SystemExit('FEHLER: ' + what)
    ok += 1; print('ok  ', what)

def boot(vw=640, vh=460, ini=None):
    g = Geos(D, vw, vh, ini=ini)
    g.setv(V['WIN'], 0x55); g.setv(V['VW'], vw); g.setv(V['VH'], vh)
    g.call('startup')
    return g

def report(g):
    seg = g.v(V['LSEG']); rep = g.rd(seg, 1868 + 12000 + 16, 8000)
    rows, i = [], 0
    while rep[i] != 0xff:
        page, kind = rep[i], rep[i + 1]; i += 2
        a = rep.index(0, i); lab = rep[i:a].decode('mac_roman'); i = a + 1
        b = rep.index(0, i); val = rep[i:b].decode('mac_roman'); i = b + 1
        rows.append((page, kind, lab, val))
    return rows

g = boot()
check(g.v(V['INIT']) == 1, 'Start: Speicher angelegt, Erkennung gelaufen')
check(g.v(V['TC']) == 1, 'Echtfarben-Treiber BB72_64 aus GEOS.INI erkannt')
g.call('repaint')
r = report(g)
val = {}
for p, k, l, v in r:
    if k == 0: val.setdefault((p, l), v)
for p, k, l, v in r:
    print('   ', p, 'H' if k else ' ', l, '|', v)
check(val[(0, 'Rechner')] == 'DOSBox', 'Emulator DOSBox an der BIOS-Kennung erkannt')
check(val[(1, 'BIOS-Kennung')].startswith('IBM COMPATIBLE 486 BIOS'), 'BIOS-Kennung ausgelesen')
check(val[(1, 'BIOS-Datum')] == '01/01/92', 'BIOS-Datum')
check('IBM AT' in val[(1, 'Modell-Byte')], 'Modell-Byte FCh')
check(val[(2, 'INT 12h')].startswith('640 KB'), 'Konventioneller Speicher 640 KB')
check('15.360 KB' in val[(2, 'Laut CMOS')], 'CMOS: 15360 KB erweitert')
check('XMS 3.00' in val[(2, 'Treiber')], 'XMS-Treiber 3.00')
check(val[(3, 'Auflösung')] == '800 x 600', 'VESA: 800 x 600')
check('65.536' in val[(3, 'Farbtiefe')], 'Farbtiefe 16 Bit = 65.536 Farben')
check(val[(3, 'Gerät')] == 'Basebox: 72 DPI 64K-color', 'GEOS.INI [screen 0] device')
check(val[(4, 'Maustreiber')] == 'CTMABS.GEO', 'GEOS.INI [mouse] driver')
check(val[(4, 'Drucker')].startswith('PDF-Datei (In Datei drucken) an LPT1, APS-PS'), 'Druckerliste aus { } verbunden')
check(val[(4, 'Autostart')] == 'welcome.geo geolos.geo', 'Autostart')
check(val[(1, 'BLASTER (Soundkarte)')] == 'A220 I7 D1 H5 T6', 'BLASTER aus der DOS-Umgebung')
check(val[(1, 'Version')].startswith('5.00 MS-DOS'), 'DOS-Version 5.00 MS-DOS')
check('frei' in val[(1, 'Speicherplatz')], 'Speicherplatz Laufwerk C:')
check(val[(4, 'KERNEL.EXE')] == '72.937 Byte', 'Größe von KERNEL.EXE')
check(g.range == (1, 2) or g.range[0] == 1, 'Seitenbereich an den Druckdialog: %r' % (g.range,))
fl = font_list(g)
for f in fl: print('   ', f)
check([f[0] for f in fl] == ['Berkeley', 'dingbats-Thin', 'LED', 'Printer 10cpi', 'URW Sans'],
      'Schriften: 5 Dateien gelesen, Nicht-Schriften übersprungen, nach Namen sortiert')
check(fl[0][1:] == ('BERKELEY.FNT', 0x0202, 0, [9, 10, 12, 14, 18]), 'Berkeley: Datei, ID 0202h, Bitmapgrößen sortiert ohne Doppelte/Fett')
check(fl[4][3] == 1 and fl[1][3] == 1 and fl[2][3] == 0, 'Umriss- und Bitmapschriften unterschieden')
check(g.enums == 1 and not g.dirs, 'FileEnum einmal, Verzeichnis zurückgesetzt')
check(val[(4, 'Schriften (Ordner FONT)')] == '5 Schriftdateien', 'Seite GEOS: Zahl der Schriften')
check(val[(9, 'Klänge der Oberfläche ([ui] sound)')] == 'true', 'Seite Klang: [ui] sound aus GEOS.INI')
for p in range(12):
    g.setv(V['PAGE'], p); g.setv(V['SCR'], 0)
    g.canvas.paste((1, 2, 3), (0, 0, 640, 460))
    g.texts = []
    g.call('repaint')
    g.canvas.save(os.path.join(OUT, 'seite%d.png' % p))
    if p == 8: t8 = list(g.texts)
    if p == 9: t9 = list(g.texts)
check(True, 'Alle 12 Seiten gezeichnet (out/seite0..11.png)')
for t in ('Schriften', 'Klang', 'Berkeley', 'BERKELEY.FNT', '514 (0202h)', 'Bitmap 9/10/12/14/18', 'Umriss (Nimbus)', 'Drucker 12', 'ÄÖÜ äöü ß 0123'):
    check(t in t8, 'Seite Schriften zeigt "%s"' % t)
for t in ('Ton A (440 Hz)', 'Alle meine Entchen', 'Xylophon', 'Instrument: Klavier – einen Knopf der oberen Reihe anklicken'):
    check(t in t9, 'Seite Klang zeigt "%s"' % t)
# sound
def play(g, limit=200):
    g.ports = []
    for _ in range(limit):
        if g.w(g.v(V['LSEG']), L_SNDRUN) == 0: break
        g.call('snd_step', far=True)
    return g.ports
def divisors(ports):
    out, i = [], 0
    while i < len(ports):
        if ports[i] == (0x43, 0xb6): out.append(ports[i + 1][1] | ports[i + 2][1] << 8); i += 3
        else: i += 1
    return out
g.setv(V['PAGE'], 9)
g.pc_msgs = []
g.call('start_select', far=True, cx=60, dx=230)
seg = g.v(V['LSEG'])
check(0x0c06 in g.pc_msgs and g.w(seg, L_SNDRUN) == 1, 'Knopf "Ton A": Klang beginnt (Nachricht 0C06h über die Warteschlange)')
pt = play(g)
check(divisors(pt) == [2712] and pt[-1][0] == 0x61 and pt[-1][1] & 3 == 0, 'Ton A: Teiler 2712 (440 Hz), Lautsprecher danach aus')
g.call('start_select', far=True, cx=60 + 151 * 2, dx=230)
check(divisors(play(g)) == [4560, 4063, 3620, 3417, 3044, 3044, 2712, 2712, 2712, 2712, 3044, 2712, 2712, 2712, 2712, 3044,
                            3417, 3417, 3417, 3417, 3620, 3620, 4063, 4063, 4063, 4063, 4560], 'Alle meine Entchen: 27 Noten')
g.call('start_select', far=True, cx=500, dx=290)
check(g.w(seg, L_INSTR) == 3, 'Instrument Xylophon gewählt')
g.call('start_select', far=True, cx=60, dx=230)
check(divisors(play(g)) == [1356], 'Xylophon: eine Oktave höher (Teiler 1356)')
g.call('start_select', far=True, cx=60 + 151 * 2, dx=290)
g.call('start_select', far=True, cx=60 + 151 * 3, dx=230)
dv = divisors(play(g))
check(len(dv) == 30 and dv[:3] == [4560, 3620, 3044], 'Akkord: C-E-G als Arpeggio (30 Töne)')
g.call('start_select', far=True, cx=60, dx=230)
dv = divisors(play(g))
check(len(dv) > 10 and set(dv) == {2712, 2712 - 2712 // 64}, 'Flöte: Vibrato')
g.call('start_select', far=True, cx=60 + 151 * 2, dx=230)
g.call('snd_step', far=True)
g.call('kbd_char', far=True, cx=ord('1'), dx=0)
check(g.w(seg, L_SNDP) == g.sym['song_stop'], 'Seitenwechsel: Lied endet nach dieser Note')
play(g)
check(g.w(seg, L_SNDRUN) == 0, 'Klang beendet')
g.call('kbd_char', far=True, cx=ord('9'), dx=0)
check(g.v(V['PAGE']) == 8, 'Taste 9 -> Schriften')
g.call('kbd_char', far=True, cx=ord('0'), dx=0)
check(g.v(V['PAGE']) == 9, 'Taste 0 -> Klang')
g.call('start_select', far=True, cx=8 + 62 * 8 + 10, dx=50)
check(g.v(V['PAGE']) == 8, 'Klick auf Reiter 9 -> Schriften')
# keys
g.setv(V['PAGE'], 0)
g.call('kbd_char', far=True, cx=ord('3'), dx=0)
check(g.v(V['PAGE']) == 2, 'Taste 3 -> Seite Speicher')
g.call('kbd_char', far=True, cx=ord('n'), dx=0)
check(g.v(V['PAGE']) == 3, 'Taste N -> nächste Seite')
g.call('kbd_char', far=True, cx=ord('8'), dx=0)
for c in (ord('a'), 0xff90, ord('Z'), 0x8a):
    g.call('kbd_char', far=True, cx=c, dx=0x0200)
g.call('kbd_char', far=True, cx=ord('q'), dx=1)     # release: ignored
g.call('start_select', far=True, cx=400, dx=300)
check(g.v(0xF4) == 1 and g.grabs == [213], 'Drücken in der Malfläche: zeichnen, Maus gegriffen (UI 213)')
g.call('mouse_ptr', far=True, cx=410, dx=305, bp=0x8104)
g.call('mouse_ptr', far=True, cx=430, dx=325, bp=0x8104)
check(g.canvas.getpixel((420, 315)) == (0, 0, 170), 'Ziehen mit gedrückter Taste zeichnet eine Linie')
g.call('drag_select', far=True, cx=430, dx=325, bp=0x8104)
check(g.w(g.v(0xE2), 1074) == 1, 'DRAG_SELECT gezählt')
g.call('end_select', far=True, cx=430, dx=325, bp=0x0100)
check(g.v(0xF4) == 0 and g.grabs == [213, 204] and g.w(g.v(0xE2), 1072) == 1, 'Loslassen: Zeichnen endet, Maus freigegeben (UI 204)')
g.call('mouse_ptr', far=True, cx=460, dx=350, bp=0x0100)
check(g.canvas.getpixel((445, 337)) == (255, 255, 255), 'Ohne Taste keine Linie')
check(g.supers[-1] == (0x100, 0x7a), 'Unbearbeitetes MSG_META_PTR geht an VisContentClass')
g.call('start_select', far=True, cx=400, dx=400)
g.call('mouse_ptr', far=True, cx=420, dx=420, bp=0x0100)
check(g.v(0xF4) == 0 and g.grabs[-1] == 204, 'Losgelassen ohne END_SELECT (BP 0100h): Ende, Freigabe')
g.call('start_select', far=True, cx=410, dx=305)
g.call('end_select', far=True, cx=410, dx=305, bp=0x0100)
px = g.canvas.getpixel((411, 306))
g.call('kbd_char', far=True, cx=0xff2a, dx=0x1400)
check(g.canvas.getpixel((411, 306)) == px == (0, 0, 170), 'Taste (z. B. Druck) löscht die Malfläche nicht')
g.call('repaint'); g.canvas.save(os.path.join(OUT, 'eingabe.png'))
seg = g.v(V['LSEG'])
check(g.w(seg, 446) == 5, 'Eingabe: 5 Tasten aufgezeichnet')
g.canvas.paste((1, 2, 3), (0, 0, 640, 460))
g.call('vis_draw', far=True, bp=0x77)
check(g.canvas.getpixel((5, 5)) != (1, 2, 3), 'MSG_VIS_DRAW zeichnet das Fenster')
# tabs by mouse
g.call('start_select', far=True, cx=8 + 62 * 6 + 10, dx=50)
check(g.v(V['PAGE']) == 6, 'Klick auf Reiter 7 -> Grafik')
# measurement
g.call('do_measure')
check(g.w(seg, 426) == 1 and g.w(seg, 428) > 0 and g.w(seg, 430) > 0, 'Messung: Rechtecke %d/s, Texte %d/s' % (g.w(seg, 428), g.w(seg, 430)))
check(g.w(seg, 442) == 1 and g.w(seg, 438) > 0, 'Datei-Test: schreiben %d KB/s, lesen %d KB/s' % (g.w(seg, 438), g.w(seg, 440)))
g.canvas.save(os.path.join(OUT, 'leistung.png'))
# save
g.call('do_save')
txt = open(os.path.join(D, 'DOCUMENT_GWTEST.TXT'), 'rb').read()
check(txt.startswith(b'GeoWorks 1.x Test - Systembericht\r\n'), 'Bericht gespeichert (ASCII, CRLF)')
check(all(c < 128 for c in txt), 'Bericht ohne Sonderzeichen (Umlaute umschrieben)')
open(os.path.join(OUT, 'GWTEST.TXT'), 'wb').write(txt)
check(os.path.getsize(os.path.join(D, 'STATE_GWTEST.TMP')) == 0, 'Testdatei STATE\\GWTEST.TMP wieder geleert')
# clipboard
g.call('do_copy')
check(g.calls.get('ui127') == 1 and g.calls.get('ui135') == 1, 'Zwischenablage: ClipboardRegisterItem aufgerufen')
# printing
g.call('print_docname', far=True, cx=0x1234, dx=0x40, bp=0x777)
check(g.docname == 'GeoWorks 1.x Test', 'Dokumentname an den Druckdialog')
g.call('menu_print', far=True, cx=0x1234, dx=0x40, bp=GSTRING)
check(0x25a in g.pc_msgs, 'Druck abgeschlossen (25Ah gesendet)')
print('   Dokumentgröße', g.docsize)
g.paper.save(os.path.join(OUT, 'druck.png'))
# 16-colour driver
ini16 = open('GEOS.INI', 'rb').read().replace(b'BB72_64.GEO', b'VGA.GEO').replace(b'Basebox: 72 DPI 64K-color', b'VGA: 640x480 16 Farben')
g2 = boot(ini=ini16)
check(g2.v(V['TC']) == 0, '16-Farben-Treiber erkannt')
print('\n%d Prüfungen bestanden' % ok)
