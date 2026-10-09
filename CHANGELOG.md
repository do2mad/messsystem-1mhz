# Projekttagebuch / Changelog – Messsystem 6× DC + 1× AC (1mhz.de)

Kurzprotokoll der Entscheidungen und Änderungen, damit der aktuelle Stand
jederzeit klar ist. Neueste Einträge oben.

## Aktueller Stand (Kurzfassung)
- **MCU:** ATmega328P (U2, DIP-28), interner 8-MHz-Oszillator, kein Quarz.
- **Kanäle:** 6× DC + 1× AC über **CD4051**-Mux (U1) auf **einen** ADC-Pin (PC0/ADC0);
  X0…X5 = DC1…DC6, X6 = AC, X7 = GND (Selbsttest).
- **Puffer:** **LM324** (U4A) als Spannungsfolger **hinter** dem Mux
  (`MUXOUT → U4A → R5 100 Ω → ADC0`). U4B–U4D ebenfalls als Follower
  beschaltet (+ auf GND). Alt./besser: MCP6004 (pin-kompatibel).
- **Referenz:** LM4040 (U5, Vorwiderstand R2 1 k) an AREF, **umschaltbar** über `VREF_MV` im Sketch:
  `2500` = 2,5 V (Vorab-Test, Standard) / `4096` = final.
- **DC-Teiler:** 1 MΩ / **100 kΩ** = ~1:11, Bereich 0–30 V, 100 nF am Knoten,
  bipolare **1N4148**-Klemme (+5 V und GND) je Kanal.
- **AC (9 V~):** Brücke D1 (MB6S / DB207 / B700C1000 – Typ noch festzulegen),
  DC− auf GND, Teiler R6 470 k / R7 150 k,
  10 nF, Mittelwert-Gleichrichtung → `Vrms = MW / 0,9`, 2-Punkt-Kalibrierung.
- **Anzeige:** LCD 20×4 (HD44780, **U3**) **parallel/4-bit** (kein I²C),
  Kontrast R4 10 k (Trimmer), Backlight R3 180 Ω.
- **Referenzbezeichner (KiCad, maßgeblich):** U1 CD4051, U2 ATmega328P,
  U3 LCD 20×4, U4 LM324 (A–E), U5 LM4040, D1 Brücke, SW1 Kalibriertaster.
  Umrechnung alt → neu am Ende von `hardware/BOM.md`.
- **Alle ICs mit Pinnummern** im Schaltplan; Mux INH(6)→GND, VDD(16)→+5 V eindeutig.
- **Einspeisung:** J9 (2-pol.) *oder* J10 (USB-Mini, nur VBUS/GND) – parallel
  auf +5 V, kein Verpolschutz, daher **nur eine Quelle gleichzeitig**. C13 100 µF.
- **Sammelstecker J8** (2×5): alle Mess-Eingänge + 5 V/GND, parallel zu J1…J7.
- **Kalibrierung per Taster SW1** (PC3/Pin 26 → GND): beim Einschalten gedrückt
  halten. 12-Schritt-Modus (erst 6× 5 V, dann 6× 12 V), **pro Kanal eigenes A/B**,
  kein Wert wird gespeichert, wenn an einem Punkt keine plausible Spannung anliegt (automatisches Überspringen); alles im **EEPROM**.

## Programmierung (TL866-II Plus)
- Arduino-IDE: MiniCore → ATmega328 / Variante 328P / Internal 8 MHz / No bootloader
  → „Kompilierte Binärdatei exportieren" → `.hex`.
- Xgpro: ATMEGA328P@DIP28, .hex in Code-Puffer, Fuses **Low=0xE2, High=0xD9,
  Ext=0xFF** (0xFD für BOD 2,7 V) → Program.

## Offene Punkte (Details: docs/Review_KiCad-Schaltplan.md)
Stand 21.09.2026 – Schaltplan und Board synchron, DRC fehlerfrei, Netzliste korrekt,
alle Netze durchgehend verdrahtet, Abstände regelkonform.

**Summer ✔:** Es bleibt beim **passiven Piezo** – eingesetzt wird der
Reichelt **EPM 121** (Piezo-Schallwandler, 4 kHz). Schaltung wie geroutet
(BZ1 gegentaktig an PB6/PB7 über R24 330 Ω), Board muss nicht geändert werden.
Rastermaß am Bauteil geprüft – passt zum Footprint `Buzzer_12x9.5RM7.6` ✔.

**Footprints, die zur Bestückung passen müssen:**
- J18 soll abgewinkelt bestückt werden → Footprint auf
  `Connector_PinHeader_2.54mm:PinHeader_2x03_P2.54mm_Horizontal` ändern
  (Pads identisch, nur Körper/Bestueckungsdruck anders).
- R4 (Kontrast) ist liegend eingebaut, der Footprint ist aber
  `Potentiometer_Bourns_3296W_Vertical` → an das tatsächliche Poti anpassen.

**Hardware V1.1 – Board geprüft (21.09.2026):** elektrisch in Ordnung, siehe
`docs/Review_KiCad-Schaltplan.md`, Abschnitt 5. Offen:
- **BZ1 (9,5 mm hoch) und J18 (ISP) liegen unter dem Display** – Höhe prüfen bzw.
  entscheiden, ob sie nach unten rechts (x 122–160, y 103–116 mm) umziehen.
- GND-Zonen sind ungefüllt gespeichert → `B`, dann DRC, dann Gerber.

**Als Nächstes in KiCad:**
1. **ERC** laufen lassen – die 16 offenen Pins sind jetzt mit „No Connect“
   markiert, das Ergebnis sollte sauber sein.
2. **Schriftfeld** ausfüllen (Title / Date / Rev).
3. **Gerber neu exportieren** – der Ordner vom 31.08. ist überholt.
4. `backup_vor_*`-Ordner aufräumen.

**D1 – geklärt ✔**
- Value `DB207`, bedrahteter DIP-4-Footprint. Pinbelegung am Bauteil geprüft:
  beide `~` links, rechts oben `−`, rechts darunter `+` – identisch zur
  Pad-Anordnung auf der Platine. Das MB6S-Symbol hat dieselbe Pinreihenfolge,
  daher keine Änderung nötig.

**Bewusst verworfen (20.09.2026):**
- Reset-Taster S2 – wird nicht gebraucht.
- Ein/Aus-Schalter – wird nicht gebraucht.

**Optional offen:**
- ISP-Header 2×3 (PB3–PB5 frei), Pads für PC1/PC2 (±-Erweiterung).

**Geprüft und in Ordnung:**
- Netzliste stimmt mit der Spezifikation überein; ATmega Pin 22 auf GND;
  USB J10 (VBUS/GND/Shield); „Plus links“ durchgängig. ✔
- Keine unverdrahteten Verbindungen; Pad-Abstand min. 0,84 mm, Bahnabstand
  min. 0,200 mm, Kupfer-zu-Rand min. 0,50 mm. ✔

## Aufbau der Beta-Platine (08/2026)
Quelle: Forum-Thread
<https://1mhz.de/forum/thread/8687-7-spannungen-messystem-mit-20x4-lcd-display/>
Die erste Platine wurde aufgebaut und läuft – mit Fädelbrücken und
Änderungen. Die dabei gefundenen Fehler und ihr heutiger Stand:

| Beim Aufbau gefunden | Stand im aktuellen KiCad-Projekt |
|---|---|
| Verdrahtung des LM324 muss angepasst werden | **erledigt** im Schaltplan: U4A als Follower mit Gegenkopplung, U4B–U4D ebenfalls als Follower, +Eingänge auf GND |
| Mini-USB-Buchse muss gedreht werden | **erledigt:** J10 mit echtem USB-Symbol, VBUS auf Pad 1, GND auf Pad 5, Shield auf GND |
| 2-mm-Bananenbuchsen tauschen – überall sonst ist **Plus links** | **erledigt:** an J1…J7, J9 und J11…J14 liegt das Plus/Signal jeweils auf dem linken Pad (nachgemessen) |
| Kalibriertaster fehlt im Layout | **erledigt:** SW1 ist in Schaltplan und Board |
| Reset-Taster vorsehen | **offen** (S2) |
| Ein-/Aus-Schalter vorsehen | **offen** |
| Gerber deshalb noch nicht veröffentlicht | gilt weiter – die vorhandenen (31.08.) sind veraltet, Neuexport nach ERC/DRC |

### Entscheidungen aus der Diskussion im Thread
- **Messadapter-Schnittstelle: 2×5-Wannenstecker** (Schneidklemm-Flachbandkabel).
  Vorher diskutiert und verworfen: RJ45 (nur 8 Pins, Schirm nötig – Vorschlag
  axorp) und 9-pol. Sub-D (1× GND, 6× DC, 2× AC). Der Wannenstecker erlaubt
  gerätespezifische Zusatzplatinen (C64, PET …) am selben Kabel.
  → Im Schaltplan als **J8** (`Conn_02x05_Odd_Even`) vorhanden.
- **Strommessung bewusst nicht integriert.** Die Platine würde zu groß; dafür
  gibt es den Commodore-PSU-Tester von Sven Petersen, den Martin bereits
  aufgebaut hat. Kandidat für eine spätere Erweiterungsplatine
  (Vorschlag axorp: 0,1 Ω / ≥2 W als Shunt, eigene RJ45-Buchse).
- **Negative Spannungen (−5…−15 V)** für 1–3 Kanäle per Jumper/DIP-Schalter:
  Idee steht, noch nicht umgesetzt – kostet zu viele Bauteile für die kleine
  Platine. Siehe `docs/Erweiterung_Negativspannung.md`.
- **Weitere Vorschläge von axorp, zurückgestellt auf eine Version 2:**
  AC-Eingang umschaltbar als 7. DC-Kanal; zweiter CD4051 + LM324 für mehr
  Kanäle; Über-/Unterspannungsüberwachung mit Saver-Schaltung (BTS3xx).
- **Anzeige:** Kanäle unter 0,2 V werden ausgeblendet. Ursprünglich wurde
  „0.00 V“ angezeigt – das machte die Anzeige unübersichtlich.
- Hinweis aus dem Thread: axorp hat gebeten, **vor** einer Neubestellung der
  Platine noch einmal Rücksprache zu halten.

## Änderungshistorie
- **08.10.2026 – Firmware V1.2:** Neue Platine aufgebaut und getestet, läuft.
  Danach drei Erweiterungen:
  **1. Toleranz am Taster wählbar.** Taste ≥ 2 s → „Grenzen einrichten",
  Start bei ± 1 %; jeder weitere Tipp +1 % (1…20, dann wieder 1); 5 s ohne
  Tipp → einlernen mit dem gezeigten Wert. Löschen weiterhin ≥ 6 s halten.
  Das Statusfeld zeigt beim Halten `SETUP` und im Einrichten die Toleranz
  (`+- 7%`). **2. Wert im EEPROM** (Adresse 120, 1 Byte + Prüfbyte), bleibt
  über den Neustart erhalten; Konsolenbefehl `t` setzt ihn direkt.
  **3. Summer per Jumper aus:** Jumper über **J18 Pin 1 und 3** (MISO/PB4 mit
  internem Pullup gegen SCK/PB5, das fest auf Low liegt) schaltet den Ton
  komplett ab; Lichtblinken und Display-Blinken bleiben. Beide Pins sind im
  Betrieb sonst unbenutzt, J17 bleibt dadurch komplett frei (PC5 weiter für
  I²C). **Jumper vor jedem Programmieren abziehen** – er verbindet sonst MISO
  und SCK. Auf der nächsten Platine bekommt das einen eigenen 2-poligen Jumper.
  Auf der nächsten Platine bekommt das einen eigenen 2-poligen Jumper.
  **4. Der Kanal mit Alarm blinkt im Display** zwischen Messwert und `ALARM`
  (500 ms, eigener Takt neben dem Hintergrundlicht) – das `!` allein war zu
  unauffällig. „Stumm" beendet auch das Blinken.
  Der Jumper-Pin wird direkt über die Portregister gelesen (PB4 mit Pull-up,
  PB5 als Low-Ausgang), damit die Pin-Nummerierung des Cores keine Rolle
  spielt; `?` zeigt den rohen Pegel als `(PB4=0/1)` an. Getestet und in
  Ordnung. **Stolperfalle beim Testen:** Die Arduino-IDE lädt eine von außen
  geänderte .ino nicht von selbst nach – vor dem Kompilieren den Sketch neu
  öffnen, sonst flasht man die alte Fassung.
  Kompiliert ohne Warnungen (22,6 KB Flash, 0,95 KB RAM).
  Vorversion: `firmware/archiv/Messsystem_Mux_V1.1_final.ino.txt`.
- **25.09.2026 – gefälschter „LM4040 4,1 V" erwischt (Aufbau V1.0):** Nach dem
  Einbau von LM4040-4.1 + MCP6004 zeigte der Aufbau bei 5 V **27,24 V** an.
  Ursache: an AREF lagen nur **0,76 V**. Der vermeintliche LM4040-4.1 ist in
  Wahrheit ein **NPN-Transistor** – der Bauteiltester (TC1) meldet BJT-NPN,
  hFE 322, Ube 636 mV, also drei aktive Beine. Bei uns hängt Pin 2 über R2 an
  +5 V und Pin 3 an GND, die Basis-Emitter-Strecke leitet → 0,76 V.
  Rechenprobe: 0,455 V am ADC bei 0,76 V Referenz = 618 Counts, × 0,0440 V/Count
  = 27,2 V. Passt exakt, eine EEPROM-Kalibrierung war nicht im Spiel.
  **Gegenprobe mit dem echten LM4040-2.5:** Tester meldet „2 Diodes",
  0,814 V in Durchlass- und **2,53 V** in Sperrrichtung (= die Referenzspannung),
  drittes Bein unbeschaltet. Pinbelegung dieses Exemplars: **1 = Anode,
  2 = Katode, 3 = NC**.
  **Merke:** Neue Referenz vor dem Einlöten in den Bauteiltester stecken – ein
  echter LM4040-4.1 muss „2 Diodes" mit ~0,8 V und **~4,1 V** zeigen. Meldet der
  Tester einen Transistor, ist es keine Referenz. Alternativ 5 V über 1 kΩ auf
  die Katode, Anode an GND, Spannung messen.
  Teil reklamieren; Nachkauf bei Distributor mit Herstellerkette.
- **25.09.2026 – MCP6004 statt LM324, Referenz auf 4,096 V:** U4 ist jetzt der
  **MCP6004** (Rail-to-Rail-CMOS, pin-kompatibel, gleiches DIP-14, **max. 6 V**),
  damit der volle Bereich der LM4040-4.096 nutzbar ist – der LM324 kam am
  Ausgang nur bis ~3,5 V. Keine Schaltungsänderung: Beschaltung, R5 (100 Ω) und
  die als Folger beschalteten Sektionen U4B–D bleiben. Wert in Schaltplan und
  Board auf `MCP6004` geändert. Firmware: `VREF_MV` von 2500 auf **4096**
  (kompiliert ohne Warnungen, 20,9 KB Flash). **Danach neu kalibrieren** – die
  EEPROM-Werte galten für die 2,5-V-Referenz.
  Sicherung: `KiCad/messystem/backup_vor_mcp6004_140100/`.
- **22.09.2026 – Projektbibliothek angelegt (Weitergabe):** alle benutzten
  Symbole (24) in `KiCad/messystem/libs/messystem.kicad_sym`, alle Footprints (24)
  in `KiCad/messystem/libs/messystem.pretty/`, eingebunden über die
  projektbezogenen Tabellen `sym-lib-table` und `fp-lib-table` (`${KIPRJMOD}`).
  Schaltplan und Board verweisen jetzt nur noch auf `messystem:…`. Footprints
  1:1 aus dem Board übernommen (Pads gegen alle 79 Bauteile geprüft), Netzliste
  unverändert. Alle 3D-Modelle liegen in `packages3D/` (13 KiCad-Standardmodelle
  ergänzt), kein Pfad zeigt mehr auf eine KiCad-Installation. Zum Weitergeben
  genügt der Ordner `KiCad/messystem/` (ohne `backup_*`).
  Sicherung: `KiCad/messystem/backup_vor_projektbibliothek_091629/`.
- **21.09.2026 – Summer: doch passiver Piezo Reichelt EPM 121.** Der Umbau
  auf den aktiven MB 12 (Q2/R25/R26/D18) wurde wieder verworfen – zu viel
  Aufwand. Schaltplan zurück auf den gerouteten Stand (BZ1 + R24 gegentaktig
  an PB6/PB7), nur BZ1-Wert jetzt „EPM 121“. Firmware: Piezo-Version wieder
  aktiv, Tonfrequenz von 2,7 auf **4 kHz** (Resonanz des EPM 121) erhöht;
  kompiliert ohne Warnungen (20,9 KB Flash, 0,9 KB RAM). Die MB12-Variante
  liegt als Referenz in `firmware/archiv/Messsystem_Mux_V1.1_summer_aktiv_MB12.ino.txt`
  und `KiCad/messystem/backup_variante_MB12/` (Schaltplan mit Q2/R25/R26/D18).
- **21.09.2026 – 3D-Modelle ergänzt:** neuer Ordner `KiCad/messystem/packages3D/`.
  Die fünf dort schon erwarteten KiCad-Standardmodelle (Widerstand, Scheiben-C,
  Elko, DIP-16-Sockel, Buchsenleiste 1×3) nachgeliefert; Stiftleisten J1–J7/J9
  auf lokale Kopie umgestellt (vorher `KICAD9`-Pfad). Eigene Modelle:
  `LCD_20x4_HD44780_12mm.wrl` (Modul 98×60 mm auf 12 mm, Buchsenleiste,
  M3-Bolzen), `USB_Mini-B_THT_Horizontal_MAD.wrl` (J10), `Banana_2mm_Pair_MAD.wrl`
  (J16, rot/schwarz). Logos bleiben ohne Modell (sind Bestückungsdruck).
  Sicherung: `KiCad/messystem/backup_vor_3d_120052/`.
- **Summer BZ1 = Visaton MB 12 – 5 V (Art. 3590):** das ist ein
  **aktiver Magnet-Summer mit eingebautem Generator** (5 V DC, 2,3 kHz, 88 dB,
  Rastermass 7,4 mm, gepolt) – **nicht** der passive Piezo, für den R24 und
  die Firmware V1.1 ausgelegt sind. Entscheidung: stattdessen EPM 121, siehe oben.
- **21.09.2026 – Board V1.1 geroutet und geprüft:** alle 59 Netze synchron zum
  Schaltplan, alle 52 Netze allein über Bahnen verbunden, Abstände min. 0,200 mm,
  43 Vias 0,6/0,3 mm, keine überlappenden Bauteile. Hinweis: BZ1 und J18 liegen
  unter dem LCD-Modul.
- **21.09.2026 – Schaltplan V1.1 erweitert:** Q1 (2N7000) mit R22/R23 schaltet
  das LCD-Licht (U3 K → Q1 Drain statt GND), BZ1 (Piezo passiv) mit R24 an
  PB6/PB7, J17 Erweiterungsleiste (+5V, PC1, PC2, PC4, PC5, GND), J18 ISP
  (Standard-AVR-Belegung). Platziert oben links, verbunden über globale Labels.
  Die 9 No-Connect-Kreuze an U2 wieder entfernt. Netzliste gegen die Vorversion
  geprüft: außer U3 Pin 16 keine bestehende Verbindung verändert.
  Sicherung: `KiCad/messystem/backup_vor_schaltplan_v1.1_093138/`.
- **21.09.2026 – Firmware V1.1** (Umsetzung der Forum-Vorschläge, siehe
  `docs/Bewertung_Vorschlaege_axorp.md`, Punkte 1–3):
  Grenzwert-Überwachung pro Kanal (Einlernen per Taste ≥ 2 s, ± 5 %,
  Löschen ≥ 6 s, kurz = stumm), `!`-Markierung und Statusfeld auf dem LCD,
  Alarm über blinkendes Licht und Piezo-Summer, Datenaufzeichnung als CSV
  (`;`, Dezimalkomma) über J15, neue Konsolenbefehle `l s g G e x m b`.
  Grenzwerte im EEPROM ab Adresse 64, Kalibrierung bleibt erhalten.
  Taster im Betrieb per Pin-Change-Interrupt, Summer per Timer1 – nichts
  blockiert die Messung. Kompiliert ohne Warnungen (~21 KB Flash, ~0,9 KB RAM).
  Alte Version: `firmware/archiv/Messsystem_Mux_V1.0/`.
- **Hardware-Plan V1.1** (noch nicht im Schaltplan): Q1 2N7000 schaltet das
  LCD-Licht (PB3), passiver Piezo BZ1 gegentaktig an PB6/PB7, Erweiterungs-
  leiste J17 (PC1/PC2/PC4/PC5, +5 V, GND), ISP-Anschluss J18. X7 bleibt GND.
  Einbauliste: `docs/Einbau_V1.1_Licht_Summer_Erweiterung.md`.
- **21.09.2026 – No-Connect-Markierungen gesetzt:** 16 bewusst offene Pins
  markiert – U2 (PB3–PB5, PC1/PC2/PC4/PC5, XTAL1/PB6, XTAL2/PB7),
  U3 (DB0–DB3) und J10 (D+, D−, ID). Die Liste wurde gegen die unverbundenen
  Pads im Board gegengeprüft und stimmt eins zu eins überein.
  Sicherung: `KiCad/messystem/backup_vor_noconnect_223749/`.
- **21.09.2026 – D1 geklärt:** Pinbelegung des DB207 am Bauteil gegengeprüft
  (beide ~ links, rechts oben −, darunter +). Stimmt mit der Pad-Anordnung
  überein, das MB6S-Symbol passt. Keine Änderung nötig.
- **21.09.2026 – DRC sauber.** Nach Wärmefallen-Spalt 0,35 mm, Zonen-Abstand
  0,30 mm, Mindest-Stegzahl 1 und durchgehender Anbindung von U2.22, U4.11,
  U4.12 läuft der DRC ohne Fehler durch.
- D1-Value auf **DB207** geändert (Footprint passte schon zum bedrahteten
  Gehäuse); Tippfehler „Kaliebriertaster“ beseitigt. Das **Symbol** ist noch
  das des MB6S – Pinreihenfolge prüfen, siehe oben.
- **20.09.2026 (21:59) – letzte vier DRC-Meldungen:** U2 Pad 22, U4 Pad 11 und
  U4 Pad 12 auf **durchgehende Zonenanbindung** gesetzt (keine Wärmefalle mehr,
  dafür keine abgeschnittenen Inseln); **Mindest-Stegzahl** von 2 auf 1 gesenkt
  für den C1-Fall. Sicherung: `KiCad/messystem/backup_vor_zoneconnect_215936/`.
  Zonen-Parameter am Ende: Abstand 0,30 mm, Wärmefallen-Spalt 0,35 mm,
  Stegbreite 0,50 mm, Mindestbreite 0,25 mm.
- **20.09.2026 (21:57) – Zonen-Abstand gesenkt:** nach dem zweiten DRC-Lauf
  noch 9 Meldungen (weiterhin 0 unverbundene Pads). Zonen-Abstand beider
  GND-Zonen von 0,50 auf **0,30 mm** (Netzklasse erlaubt 0,20 mm), damit die
  Fläche zwischen den Bahnen zusammenwächst und die isolierten Inseln
  verschwinden. Sicherung: `KiCad/messystem/backup_vor_zonenabstand_215657/`.
- **20.09.2026 (21:52) – Wärmefallen entschärft:** nach dem ersten DRC-Lauf
  11 `starved_thermal`-Meldungen (0 unverbundene Pads). Wärmefallen-Spalt
  beider GND-Zonen von 0,50 auf **0,35 mm** verkleinert, Stegbreite bleibt
  0,50 mm. Sicherung: `KiCad/messystem/backup_vor_thermal_215252/`.
  Falls die sieben „isolierte Insel“-Fälle bleiben: Zonen-Abstand von 0,5
  auf 0,3 mm senken.
- **20.09.2026 (21:35) – GND-Masseflächen eingefügt:** zwei Zonen auf Netz GND,
  je eine auf B.Cu und F.Cu, Rechteck über die ganze Platine, 0,25 mm vom Rand
  eingerückt. Mindestbreite 0,25 mm, Zonenabstand 0,5 mm, Wärmefallen 0,5/0,5 mm.
  **Noch zu füllen** (Taste `B` in PCBnew). Sicherung:
  `KiCad/messystem/backup_vor_gnd_pour_20260920_213534/`.
- **Zusätzliche 2-mm-Bananenbuchse für DC6** als **J16 „DC6-2“** in Schaltplan
  und Board annotiert (vorher zwei lose Pads ohne Symbol).
- **Reset-Taster und Ein/Aus-Schalter verworfen** – werden nicht gebraucht.
- **20.09.2026 (21:09) – Serieller Port ergänzt:** J15 (1×3, GND/TX/RX) an
  PD1/PD0. Damit ist die serielle Kalibrierkonsole aus der README erreichbar.
  J9 von „DC3“ auf „5V“ korrigiert. Zusätzliche 2-mm-Buchse für DC6 als
  zwei Einzel-Pads im Board (noch nicht im Schaltplan). Pinbeschriftung für
  D1 auf den Bestückungsdruck gesetzt.
- Werte der Sammelbuchsen vergeben: J11/J12 = „5V/AC“, J13/J14 = „DC1-DC6“
  (vorher standen dort die Footprint-Namen).
- **20.09.2026 – Schaltplan und PCB überarbeitet:** USB-Buchse als J10 mit
  korrektem Symbol, Sammelstecker J8 (2×5 IDC) im Board, die vorher
  unannotierten Buchsenleisten als J11/J12 (1×4) und J13/J14 (1×12)
  aufgenommen, „Plus links“ durchgängig umgesetzt, Board neu verdrahtet
  (617 Segmente, 11 Vias). Geprüft: keine unverdrahteten Verbindungen,
  Abstände regelkonform.
- **09/2026 – KiCad-Schaltplan gezeichnet** (`hardware/Messystem_SchaltPlan-KiCad.pdf`,
  KiCad 10.0.3). Gegen die Spezifikation geprüft: Mux-Belegung X0…X5 = DC1…DC6,
  X6 = AC, X7 = GND; Select A/B/C = PD2/PD3/PD4; Puffer U4A als Follower →
  R5 100 Ω → PC0; DC-Teiler 1 M/100 k mit bipolaren Klemmen; AC-Teiler
  470 k/150 k hinter der Brücke; LCD 4-bit auf PD5/PD6/PD7/PB0/PB1/PB2.
  **Schaltplan: alles korrekt umgesetzt** – Netzliste aus dem Board
  ausgewertet, kein Verdrahtungsfehler. Offene Punkte liegen im PCB.
  Prüfbericht: `docs/Review_KiCad-Schaltplan.md`.
- **BOM und README auf die KiCad-Bezeichner umgestellt** (der KiCad-Plan ist
  ab jetzt die Quelle der Wahrheit). Querverweistabelle alt → neu in `BOM.md`.
  Achtung: der Hinweis „R7 470 k → 820 k“ beim 2,5-V-Vorabtest betrifft jetzt **R6**.
- Design-Notiz docs/Erweiterung_Negativspannung.md ergänzt (Negativspannung,
  +/- per Schalter umschaltbar – für später, nach Einbau der LM4040-4.096).
- Glättung verbessert: unter der Deadband-Schwelle sauber auf 0 (blendet wieder
  aus), große Sprünge (C64 ein/aus) folgen sofort, nur kleines Zappeln wird geglättet.
- DC_DEADBAND auf 0,20 V (offene Eingänge fingen ~0,1 V Brumm ein).
- Leichte Anzeige-Glättung (EMA, SMOOTH=0.25) gegen ~0,05 V Zappeln; Abtastung
  unverändert. Starttext angepasst (1MHz.de / 09.2026 @mad).
- Option HIDE_ZERO: 0-V-Kanäle werden ausgeblendet (nur aktive Kanäle zeigen
  einen Wert). Per #define abschaltbar. Hinweis: tote Schiene (0 V) sieht dann
  aus wie unbenutzt.
- Nullunterdrückung: Beträge < 0,10 V (DC/AC) werden als 0,00 angezeigt
  (Rest-Offset nach Kalibrierung wird nicht mehr als 0,04–0,06 V angezeigt).
- Kalibrierung: bei 5 V übersprungene Kanäle werden in der 12-V-Runde nicht
  mehr abgefragt (spart Schritte).
- Anzeige-Fix: Lücke zwischen den Spalten wird jetzt mitgelöscht (keine Text-
  Reste der Kalibrieranzeige mehr). Hinweis, wenn 0 Kanäle kalibriert wurden
  (je 5 V UND 12 V pro Kanal nötig).
- Reset-Taster **S2** ergänzt (RESET/Pin 1 → GND). Kalibrier-Fenster auf ±20 % verschärft.
- Kalibrierung auf 12-Schritt-Modus umgestellt (erst 6× 5 V, dann 6× 12 V),
  pro Kanal eigenes A/B. Überspringen automatisch: liegt keine plausible
  Kalibrierspannung an, wird der Punkt ignoriert (einzelne Kanäle nachkalibrierbar).
- Anzeige neu sortiert: 1/2/3 links, 4/5/6 rechts, mit „V", rechte Spalte rechtsbündig.
- Taster-Kalibrierung (5/12 V an DC1, Taste S1 an PC3) + EEPROM-Speicherung ergänzt.
- Firmware: serielle Konsole mit 2-Punkt-Kalibrierung (`?`, `r/v`, `1..6`, `a`).
- Referenz umschaltbar gemacht (`VREF_MV`), Standard 2500 für Vorab-Test mit LM4040-2.5.
  Hinweis: mit 2,5 V clippt der 9-V~-Kanal leicht (genau bis ~8 V~); für volle
  9–12 V~ entweder LM4040-4.096 (final) oder AC-Teiler R7 470 k→820 k.
- DC-Teiler-Unterwiderstand von 110 k auf **100 k** geändert (bestückt) → 1:11.
- Referenz/OpAmp renummeriert: LM4040 = **U4** (vorher doppelt U3); R/C lückenlos.
- Pinnummern an allen ICs ergänzt; Mux INH/VDD als klare Netz-Labels.
- **LM324-Pufferverstärker** (U3A) hinter dem Mux ergänzt (Impedanzwandler).
- Bipolare 1N4148-Klemmen statt BAT85; Eingänge hochohmiger (1 M).
- Umstieg von 3× ADS1115 (isoliert) auf ATmega-internen ADC + CD4051-Mux;
  AC ohne galvanische Trennung (potentialfreie 9-V~-Wicklung, im C64 auf GND bezogen).
- LCD von I²C auf direkte Parallel-Ansteuerung (4-bit) umgestellt.

## Projektdateien
- `hardware/Messystem_SchaltPlan-KiCad.pdf` – **aktueller Schaltplan** (KiCad, A4)
- `hardware/Messsystem_328P_CD4051_Schaltplan.pdf` – alter Entwurfsplan (A3, historisch)
- `KiCad/messystem/` – KiCad-Projekt (Schaltplan, PCB, Gerber, Bibliothek)
- `docs/Review_KiCad-Schaltplan.md` – Prüfbericht zu Schaltplan und PCB
- `firmware/Messsystem_Mux/Messsystem_Mux.ino` – Arduino-Sketch
- `hardware/BOM.md`, `hardware/BOM.csv` – Stückliste
- `tools/schematic.py` – erzeugt das Schaltplan-PDF
- `README.md` – Gesamtdoku
