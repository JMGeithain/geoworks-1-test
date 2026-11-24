# SDK-Erweiterung: eigene Vis-Klasse mit Mausgriff (GeoWorks 1.x)

Stand: GeoWorks 1.x Test, 23.11.2026. Herausgefunden an KIDSDRAW.GEO
(Kindermaler), MIMIC.GEO, PUZZLE.GEO und UI.GEO der deutschen
GeoWorks-1.28-Installation.

**In GeoWorks 1.28 bestätigt:**
- Freihandzeichnen mit gedrückter Maustaste läuft lückenlos.
- `END_SELECT` und `DRAG_SELECT` kommen an.
- Die Tastatur erreicht das Objekt.
- Kein Absturz mehr.

Auch die Fenstergröße ist gelöst: Die View füllt das Fenster, wenn das Objekt
größer ist als jeder Bildschirm (siehe Abschnitt 3).

## Das Problem

Ist der **Prozess** das Ziel (Output) der GenView, bekommt er bei gedrückter
Maustaste nur ein, zwei `MSG_META_PTR`, kein `MSG_META_DRAG_SELECT` und kein
`MSG_META_END_SELECT`. Freihandzeichnen geht so nicht. Bewegungen ohne
gedrückte Taste kommen dagegen lückenlos an.

Programme, die echtes Ziehen können (Kindermaler, Symbol-Editor), haben ein
**eigenes Vis-Objekt** als Output der View. Das Objekt **greift die Maus**
beim Drücken und gibt sie beim Loslassen wieder frei.

## Die Bausteine (wie KIDSDRAW)

### 1. Klassenstruktur in DGROUP

KIDSDRAW hat sie bei DGROUP 0x7C, GWTEST bei DGROUP 0x100 (an die 256 Byte
angehängt):

| Offset | Wert | Bedeutung |
|---|---|---|
| +0 | `00A3, 00A3` | Oberklasse = UI-Export A3h (VisContentClass), Relocs `11h`@+0, `12h`@+2 |
| +4 | `0004` | (wie KIDSDRAW) |
| +6 | n | Zahl der Meldungen |
| +8 | `0056` | Instanzgröße |
| +A..+E | 0 | |
| +10 | `0300` | Klassenflags |
| +12 | n × Meldung | Meldungsnummern |
| danach | n × (Offset, 0002) | Handler im Code (Ressource 2), Reloc `22h` auf jedes Segmentwort |

Die Klasse wird als **Export 0** eingetragen (`exports => [[1, 0x100]]`).

**Stapelgrenze anpassen:** In DGROUP steht bei +14h das Stapelende, also
das Ende der initialisierten Daten. Bei KIDSDRAW ist das 1CAh, bei PUZZLE
292h. Das SDK schreibt dort fest 100h. Wird DGROUP für die Klasse
verlängert, muss dieser Wert die neue Länge bekommen. Sonst benutzt der
Kernel die Klasse als Stapel, und GeoWorks stürzt nach dem ersten Zeichnen
mit „bad handle passed to system“ ab.

### 2. Das Objekt

Es steht in einem eigenen Objektblock: Ressourcen-Flags `041Ah`, Reloc
`[23h,0,0]`, Chunk 22h mit Flag 3.

- Klassenzeiger `6001h, 0000h` bedeutet „eigene Klasse, Export 0“. MIMIC und
  PUZZLE nutzen `6001h, n` für Export n.
- Die Instanzdaten (92 Byte) stammen von KIDSDRAW:
  - Bounds liegen bei Byte 6–13.
  - Der View-Verweis (Chunk, Ressource) steht unausgerichtet bei Byte 37:
    `0046h, 100Dh` wird zu `0028h, 1003h`.

### 3. Die View

- **Output-Feld (+42h/+44h):** zeigt auf das Objekt, `0022h, 100Bh`, also
  Chunk 22h in Ressource 11.
- **Attribut +2Eh = D010h** (wie PUZZLE).
  - Mit 8000h, dem SDK-Standard, greift der Mausgriff nicht.
  - Mit C000h, 9000h oder 8010h geht es auch nicht: nur kurze Striche, die
    Zähler zählen nicht oder falsch, das Fenster bleibt grau.
- **Größe der View:** Sie wird nicht größer als das, was das Objekt als
  Größe meldet, also seine Bounds.
  - Die Bounds werden deshalb sehr groß gesetzt: 0, 0, 3999, 2999.
  - In GeoWorks 1.28 bestätigt (2000 × 1500 und 4000 × 3000, Vollbild bei
    1920 Pixel Breite): Das Fenster wird ganz ausgefüllt, und die Maus
    funktioniert.
  - Die Vis-Flags „so breit/hoch wie die View“ (Vis+2Dh, 80h/40h) helfen
    nicht.

### 4. Meldungen an das Objekt

Beim Eintritt zeigt `*DS:SI` auf das Objekt, `ES:DI` auf die Klasse
(ES = DGROUP).

| Meldung | Inhalt | Antwort |
|---|---|---|
| 2071h `MSG_VIS_DRAW` | BP = GState (VisContent erzeugt ihn bei `MSG_META_EXPOSED`) | |
| 00C3h `START_SELECT` | | 8000h; dabei **UI-Export 213** aufrufen (`LC 0,213`, `*DS:SI` = Objekt) → Maus greifen |
| 00CBh `DRAG_SELECT` | | 8000h |
| 00C4h `END_SELECT` | | 8000h; dabei **UI-Export 204** aufrufen (`LC 0,204`) → Maus freigeben |
| 007Ah `MSG_META_PTR` | BP = 8104h bei gedrückter linker Taste, 0100h ohne | 8800h (wie KIDSDRAW) |
| 007Bh `KBD_CHAR` | | wie bisher |

**Wichtig:** Jeder Handler muss mit `DS` = Objektblock zurückkehren
(`*DS:SI` wie beim Eintritt). Mit `DS` = DGROUP stürzt GeoWorks nach dem
ersten Zeichnen ab: „bad handle passed to system“. Der Kernel gibt den Block
über `DS` wieder frei. In GWTEST erledigt das `vis_ret`.

Unbearbeitete Meldungen gehen an die Oberklasse: `ES:DI` = eigene Klasse,
`DS:*SI` = Objekt, `KCALL 489`.

Zum Zeichnen außerhalb von `MSG_VIS_DRAW`: `MSG_VIS_VUP_CREATE_GSTATE` (2063h)
per ObjMessage (DI = 8080h) an das eigene Objekt schicken, die Antwort steht
in BP. Den Block-Handle des Objekts liefert `GeodeGetResourceHandle` (Kernel
150), der Chunk ist 22h.

## Umsetzung in GWTEST

- `tools/postbuild.pl` liest die Tabelle `G1VISCLS` aus dem Code, baut daraus
  Klasse, Export, Objektblock und View-Output.
- `app/gwtest.nasm`:
  - Tabelle `vis_class_table`
  - Makro `VIS_ENTER`
  - `vis_super`, `vis_draw`, `grab_mouse` / `release_mouse`
  - `begin_direct` über 2063h
