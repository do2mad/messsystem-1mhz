# Stückliste (BOM) – Messsystem 6× DC + 1× AC · ATmega328P + CD4051

Bezeichner **nach dem KiCad-Plan** `Messystem_SchaltPlan-KiCad.pdf`
(KiCad 10.0.3, Stand 09.2026). Der KiCad-Plan ist ab sofort die Quelle der
Wahrheit — ältere Listen mit der Nummerierung U1 = ATmega / R1a…R6a sind
überholt, siehe Querverweis am Ende.

Preise sind grobe Richtwerte (Reichelt/AliExpress), keine Tagespreise.

## Halbleiter
| Menge | Bezeichner | Bauteil | Gehäuse | Funktion / Hinweis |
|---|---|---|---|---|
| 1 | U1 | CD4051BE | DIP-16 | 8:1-Analog-Multiplexer |
| 1 | U2 | ATmega328P-PU | DIP-28 | MCU, interner 8-MHz-Osz. |
| 1 | U3 | LCD 20×4, HD44780 | Modul | Display (parallel, 4-bit) + 16-pol. Leiste |
| 1 | U4 | **MCP6004** | DIP-14 | Quad-OpAmp CMOS, Rail-to-Rail, Sektion A = Puffer. **Max. 6 V!** Pin-kompatibel zum LM324 (Vorgänger) |
| 1 | U5 | LM4040DIZ-4.1 | TO-92 | Spannungsreferenz 4,096 V (Vorabtest: LM4040-2.5, dann `VREF_MV 2500`) |
| 1 | D1 | Brückengleichrichter | **Konflikt!** | Value im Plan = **MB6S** (SOP-4, SMD), zugewiesener Footprint = **Diode_Bridge_DIP-4** (bedrahtet). Typ festlegen und Pinbelegung gegen Datenblatt prüfen (Symbol: 1/2 = ~, 3 = +, 4 = −) |
| 14 | D2, D3, D6…D17 | 1N4148 | DO-35 | bipolare Klemmdioden (je Kanal 2×) |

Klemmdioden je Kanal (a = nach +5 V, b = nach GND):

| Kanal | nach +5 V | nach GND |
|---|---|---|
| AC | D3 | D2 |
| DC1 | D14 | D15 |
| DC2 | D16 | D17 |
| DC3 | D12 | D13 |
| DC4 | D6 | D7 |
| DC5 | D8 | D9 |
| DC6 | D10 | D11 |

## Widerstände  (Metallschicht 1 %, 0,25 W)
| Menge | Bezeichner | Wert | Funktion |
|---|---|---|---|
| 1 | R1 | 10 kΩ | Reset-Pullup (RESET/PC6, Pin 1) |
| 1 | R2 | 1 kΩ | Vorwiderstand Referenz U5 |
| 1 | R3 | 180 Ω | LCD-Hintergrundbeleuchtung (A/Pin 15) |
| 1 | R4 | 10 kΩ | **Trimmer**, LCD-Kontrast — Footprint: Bourns **3296W, stehend** |
| 1 | R5 | 100 Ω | Serienwiderstand OpAmp → ADC |
| 1 | R6 | 470 kΩ | AC-Teiler oben |
| 1 | R7 | 150 kΩ | AC-Teiler unten |
| 6 | R11, R13, R15, R19, R20, R21 | 1 MΩ | DC-Teiler oben |
| 6 | R10, R12, R14, R16, R17, R18 | 100 kΩ | DC-Teiler unten (→ ≈1:11) |

DC-Teiler kanalweise:

| Kanal | 1 MΩ (oben) | 100 kΩ (unten) | 100 nF |
|---|---|---|---|
| DC1 | R20 | R17 | C9 |
| DC2 | R21 | R18 | C10 |
| DC3 | R19 | R16 | C8 |
| DC4 | R11 | R10 | C5 |
| DC5 | R13 | R12 | C6 |
| DC6 | R15 | R14 | C7 |

## Kondensatoren
| Menge | Bezeichner | Wert | Typ | Funktion |
|---|---|---|---|---|
| 1 | C1 | 1 µF | Keramik/Folie | VREF-Filter |
| 1 | C2 | 100 nF | Keramik | VREF-Filter (nah an AREF) |
| 1 | C3 | 10 nF | Keramik | AC-Knoten (HF-Filter, **nicht vergrößern**) |
| 6 | C5…C10 | 100 nF | Keramik X7R | Puffer/Filter DC-Teilerknoten |
| 3 | C4, C11, C12 | 100 nF | Keramik | Abblockung je IC – direkt an VCC von U1, U2, U4 |
| 1 | C13 | 100 µF | Elko | Stützkondensator +5 V |

## Steckverbinder / Mechanik
| Menge | Bezeichner | Bauteil | Hinweis |
|---|---|---|---|
| 1 | J1 | 2× Bananenbuchse 2 mm (Raster 2,54) | AC-Eingang 9 V~ (AC1/AC2) |
| 6 | J2…J7 | je 2× Bananenbuchse 2 mm | DC1…DC6. **Plus links** anordnen (wie bei den übrigen Anschlüssen) – auf der Beta-Platine war es anders herum |
| 1 | J8 | IDC-Wanne 2×5, RM 2,54 | Sammelstecker, **parallel** zu J1…J7 — nicht gleichzeitig belegen |
| 1 | J9 | Stiftleiste 1×2, RM 2,54 | +5-V-Einspeisung |
| 1 | J10 | USB-Mini-B-Buchse, liegend | +5-V-Einspeisung (VBUS/GND, Schirm auf GND) |
| 2 | J13, J14 | Buchsenleiste 1×12, RM 2,54 | „DC1-DC6“: je Kanal Signal (links) + GND (rechts) |
| 2 | J11, J12 | Buchsenleiste 1×04, RM 2,54 | „5V/AC“: +5 V, GND, AC1, AC2 |
| 1 | J15 | Stiftleiste 1×3, RM 2,54 | serielle Konsole: 1 = GND, 2 = TX (PD1), 3 = RX (PD0) |
| 1 | J16 | 2× Bananenbuchse 2 mm | „DC6-2“: zweite Abgriffmöglichkeit für DC6 |
| 1 | SW1 | Taster (Kurzhub, Schließer) | Kalibrier-Taster, PC3 (Pin 26) → GND |
| 1 | – | IC-Sockel-Set DIP-28 / -16 / -14 | optional, empfohlen |
| 1 | – | Platine / Lochraster + Kleinteile | – |

### J8 – Belegung
| Pin | Netz | | Pin | Netz |
|---|---|---|---|---|
| 1 | +5 V | | 2 | DC1 |
| 3 | AC1 | | 4 | DC2 |
| 5 | AC2 | | 6 | DC3 |
| 7 | DC6 | | 8 | DC4 |
| 9 | GND | | 10 | DC5 |

> **Achtung:** AC1/AC2 liegen direkt neben +5 V und den DC-Netzen. Beim
> Konfektionieren des Kabels sorgfältig arbeiten – ein Versatz legt 9 V~
> auf die 5-V-Schiene.

## Noch offen (siehe docs/Review_KiCad-Schaltplan.md)
- **PCB ist älter als der Schaltplan** → „Update PCB from Schematic“ fällig;
  erst danach stimmt diese Liste mit dem Board überein. Gerber vom 31.08.
  sind veraltet.
- **S2 Reset-Taster** (RESET/Pin 1 → GND): im CHANGELOG erwähnt, in Plan und
  Board nicht vorhanden.
- **Serial-Header** (PD0/PD1/GND) für die serielle Kalibrierkonsole.
- **ISP-Header** 2×3 (optional, PB3–PB5 sind frei; spart das Ziehen des DIP-28).
- **Brückentyp D1** endgültig festlegen (Value/Footprint widersprechen sich).
- Die vier unannotierten Buchsenleisten im PCB als Symbole in den Schaltplan
  aufnehmen – sonst verschwinden sie beim nächsten Abgleich.

## Ergänzungen V1.1 (im Schaltplan, Board noch offen)
| Menge | Bezeichner | Bauteil | Funktion |
|---|---|---|---|
| 1 | Q1 | 2N7000, TO-92 | schaltet das LCD-Licht (Gate an PB3) |
| 1 | R22 | 100 Ω | Gate-Vorwiderstand Q1 |
| 1 | R23 | 100 kΩ | Gate-Pulldown Q1 |
| 1 | R24 | 330 Ω | Vorwiderstand Summer |
| 1 | BZ1 | Piezo-Schallwandler **passiv**, Reichelt **EPM 121** (4 kHz) | Alarmton, gegentaktig an PB6/PB7 |
| 1 | J17 | Stiftleiste 1×6, RM 2,54 | Erweiterung: +5 V, PC1, PC2, PC4, PC5, GND |
| 1 | J18 | Stiftleiste 2×3, RM 2,54 | ISP (Standard-AVR-Belegung) |

Einbauliste: `docs/Einbau_V1.1_Licht_Summer_Erweiterung.md`

## Bemerkungen
- **Kein Quarz** nötig (interner 8-MHz-Oszillator).
- **1 % Metallschicht** in den Teilern hält die Toleranz klein; die 2-Punkt-
  Kalibrierung gleicht den Rest ohnehin aus.
- J9 und J10 speisen parallel auf +5 V — **nur eine Quelle gleichzeitig**
  anschließen (kein Verpolschutz, keine Entkopplung).
- **U4 = MCP6004** (seit 25.09.2026): gleiches DIP-14-Footprint wie der
  LM324, voller 0–4,096-V-Bereich, praktisch kein Offset. **Nur 5 V, max. 6 V**
  Betriebsspannung, CMOS → ESD-Vorsicht, Sockel benutzen.

## Querverweis alt → neu
| alt (bis 09/2026) | neu (KiCad) |
|---|---|
| U1 ATmega328P | **U2** |
| U2 CD4051 | **U1** |
| U3 LM324 | **U4** |
| U4 LM4040 | **U5** |
| J10 LCD-Modul | **U3** |
| GL1 Brücke | **D1** |
| R1a…R6a 1 M | **R20, R21, R19, R11, R13, R15** |
| R1b…R6b 100 k | **R17, R18, R16, R10, R12, R14** |
| R7 470 k | **R6** |
| R8 150 k | **R7** |
| R9 10 k Reset | **R1** |
| R10 1 k Referenz | **R2** |
| R11 180 Ω Backlight | **R3** |
| R12 100 Ω ADC | **R5** |
| RV1 10 k Kontrast | **R4** |
| C1…C6 Teilerknoten | **C9, C10, C8, C5, C6, C7** |
| C7 10 n AC | **C3** |
| C8 100 n VREF | **C2** |
| C9 1 µF VREF | **C1** |
| C10…C12 Abblockung | **C4, C11, C12** |
| S1 Kalibriertaster | **SW1** |
| J1…J7 | J1 = AC, **J2…J7 = DC1…DC6** |
| – | neu: **J8, J9, J10 (USB), C13** |

## Grobe Kosten
ATmega328P ~1,5–3 € · CD4051 ~0,4 € · LM4040 ~0,6 € · MCP6004 ~0,7–1 € ·
LCD 20×4 ~6–9 € · Trimmer/Vorwiderstand ~0,3 € · Brücke ~0,15 € ·
Passives/1N4148/Klemmen/Platine ~10–15 €. **≈ 20–30 €.**
