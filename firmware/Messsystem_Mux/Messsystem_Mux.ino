/* =====================================================================
 *  Messsystem  -  6x DC-Voltmeter (0-30 V) + 1x AC (9 V~, gleichgerichtet)
 *  Version 1.1  (09.2026)
 *
 *  MCU  : ATmega328P @ 8 MHz (interner Oszillator), interner 10-bit-ADC
 *  Mux  : CD4051 (8:1) -> 7 Signale auf EINEN ADC-Pin (PC0/ADC0/A0)
 *  Puffer: MCP6004-Spannungsfolger hinter dem Mux (Rail-to-Rail, pinkompatibel
 *          zum LM324, max. 6 V Betriebsspannung) -> ADC-Quelle niederohmig
 *  LCD  : 20x4, HD44780 DIREKT (parallel, 4-bit) - kein I2C-Modul
 *  Ref  : LM4040 an AREF - Variante via VREF_MV (2500=2.5V-Test / 4096=final)
 *  Kal. : Taster an A3 -> 2-Punkt-Kalibrierung (5/12 V) mit EEPROM-Speicherung
 *
 *  NEU in V1.2: Toleranz beim Einlernen per Taster waehlbar (1..20 %),
 *  im EEPROM gespeichert; Summer per Jumper komplett abschaltbar;
 *  der Kanal mit Alarm blinkt im Display mit (Wert <-> "ALARM").
 *
 *  NEU in V1.1 (Vorschlaege aus dem Forum, Thread 8687):
 *   - Hintergrundlicht ueber Q1 (2N7000) an PB3 -> blinkt bei Alarm
 *   - Piezo-Summer BZ1 (passiv, Reichelt EPM 121, 4 kHz), gegentaktig
 *     zwischen PB6 und PB7 ueber R24 330R
 *   - Grenzwert-Ueberwachung pro Kanal, Einlernen per Taste
 *   - Datenaufzeichnung (CSV) ueber die serielle Schnittstelle (J15)
 *  Laeuft auch auf der Beta-Platine ohne Q1/Summer: die Zusatzpins sind
 *  dort unbeschaltet, es fehlen dann nur Lichtblinken und Ton.
 *
 *  Bedienung des Tasters SW1 im Betrieb:
 *     kurz tippen      -> Alarmton stumm (bis der Alarm vorbei ist)
 *     >= 2 s halten    -> GRENZEN EINRICHTEN, Start bei +-1 %
 *                         danach jeder Tipp +1 % (1..20, dann wieder 1)
 *                         5 s nicht tippen -> einlernen mit dem gezeigten Wert
 *     >= 6 s halten    -> alle Grenzen LOESCHEN
 *     Beim Einschalten gedrueckt halten -> Kalibrierung (wie bisher)
 *  Die Statusanzeige unten rechts zeigt beim Halten, was beim Loslassen
 *  passiert ("SETUP " bzw. "LOESCH"), und im Einrichten den Wert ("+- 7%").
 *  Der zuletzt benutzte Prozentwert steht im EEPROM.
 *
 *  Summer dauerhaft stumm: Jumper am ISP-Stecker J18 ueber Pin 1 und 3
 *  (MISO/PB4 gegen SCK/PB5, das fest auf Low liegt). Jumper gesteckt =
 *  kein Ton; das Lichtblinken bei Alarm bleibt. Zum Programmieren muss
 *  der Jumper ab, sonst sind MISO und SCK kurzgeschlossen.
 *
 *  KEINE galvanische Trennung: die 9 V~ (potentialfreie Trafowicklung,
 *  im C64 ohnehin auf Masse bezogen) werden mit einer eigenen Bruecke
 *  gleichgerichtet, DC- auf GND, und als 7. Kanal gemessen.
 *
 *  Mux-Kanaele (CD4051 X0..X7):
 *     0..5 = DC1..DC6      6 = AC (gleichgerichtet)      7 = frei (GND)
 *  Select: A=PD2, B=PD3, C=PD4   (A = LSB)
 *
 *  Arduino-IDE:
 *    Board : "ATmega328" (MiniCore) -> Clock: Internal 8 MHz
 *    Lib   : LiquidCrystal (im Core enthalten, kein Extra-Download)
 *
 *  Projekt 1mhz.de
 * ===================================================================== */

#include <LiquidCrystal.h>   // parallel HD44780 (4-bit), im Core enthalten
#include <EEPROM.h>            // Kalibrier- und Grenzwerte dauerhaft speichern

// LCD parallel: RS,E,D4,D5,D6,D7  =  D5,D6,D7,D8,D9,D10  (PD5,PD6,PD7,PB0,PB1,PB2)
LiquidCrystal lcd(5, 6, 7, 8, 9, 10);

#define CAL_BUTTON A3   // Taster SW1 gegen GND (PC3 / Pin 26), interner Pull-up

// ---- Mux-Steuerpins -------------------------------------------------
#define MUX_A  2      // PD2  (LSB)
#define MUX_B  3      // PD3
#define MUX_C  4      // PD4
#define ADC_PIN A0    // PC0 = Pufferausgang (U4A) -> ADC0

#define CH_AC   6     // Mux-Kanal des AC-Zweigs
#define N_CH    7     // 6x DC + AC  (Index 0..5 = DC1..DC6, 6 = AC)

// ---- NEU V1.1: Hintergrundlicht und Summer --------------------------
#define BL_PIN        11      // PB3 (Pin 17) -> R22 100R -> Gate Q1 (2N7000)
                              // Q1 schaltet LCD-Kathode (K, Pin 16) nach GND
#define BL_BRIGHT    255      // Helligkeit normal (0..255, PWM ueber Timer2)
#define BL_ALARM_LOW   0      // Helligkeit in der dunklen Blinkphase
// Passiver Piezo (Reichelt EPM 121) gegentaktig zwischen PB6 (Pin 9) und PB7 (Pin 10), in Reihe
// R24 330R. Ton kommt aus dem Timer1-Interrupt -> blockiert die Messung nicht.
#define BUZ_FREQ    4000UL    // Hz; Resonanz des EPM 121 = 4 kHz -> am lautesten

// ---- NEU V1.2: Summer per Jumper stumm schalten ----------------------
// Direkt ueber die Portregister (PB4/PB5), damit die Pin-Nummerierung des
// gewaehlten Cores keine Rolle spielt:
//   PB4/MISO (IC-Pin 18) = J18 Pin 1, Eingang mit internem Pull-up
//   PB5/SCK  (IC-Pin 19) = J18 Pin 3, Ausgang fest auf Low (Masse fuer den Jumper)
                               // Jumper ueber J18 Pin 1 und 3 = Summer aus
                               // (die beiden Pins liegen nebeneinander).
                               // Das Lichtblinken bei Alarm bleibt.
                               // ACHTUNG: Jumper VOR dem Programmieren abziehen,
                               // er verbindet sonst MISO und SCK des ISP.
                               // Im Betrieb sind beide Pins sonst unbenutzt.

// ---- NEU V1.1: Grenzwerte / Alarm -----------------------------------
#define LIM_TOL_PCT      5.0f  // Vorgabe beim allerersten Start: +- 5 %
#define LIM_PCT_MIN        1   // NEU V1.2: Einstellbereich der Toleranz
#define LIM_PCT_MAX       20
#define SETUP_IDLE_MS  5000UL  // im Einrichten so lange nicht tippen -> einlernen
#define BLINK_MS        500UL  // NEU V1.2: Blinktakt des Alarmkanals im Display
#define LIM_MIN_ABS      0.15f // ... aber mindestens +- 0,15 V (Rauschen bei kleinen Werten)
#define ALARM_DELAY_MS 1000UL  // so lange muss ein Wert ausserhalb liegen (Einschaltspitzen)
#define HOLD_LEARN_MS  2000UL  // Taste so lange halten -> Grenzen einlernen
#define HOLD_CLEAR_MS  6000UL  // Taste so lange halten -> Grenzen loeschen

// ---- NEU V1.1: Datenaufzeichnung ------------------------------------
#define LOG_MS         1000UL  // Intervall der CSV-Zeilen
#define LOG_SEP          ';'   // Trennzeichen (';' passt zu dt. Excel/LibreOffice)
#define LOG_KOMMA          1   // 1 = Dezimalkomma (12,05), 0 = Dezimalpunkt (12.05)

// ---- Oversampling ---------------------------------------------------
#define N_DC    64    // Mittelungen pro DC-Kanal
#define N_AC   512    // Samples fuers AC-Mittel (~57 ms = mehrere Perioden)
#define SMOOTH 0.25f // leichte Anzeige-Glaettung (0..1; kleiner = ruhiger, aber traeger)

// ---- Teilerwerte / Referenz ----------------------------------------
// ===== REFERENZ-VARIANTE (LM4040 an AREF) HIER WAEHLEN =====
//   2500 = LM4040-2.5   (Vorab-Test: DC voll ok bis ~27,5 V;
//                        AC genau bis ~8 V~, 9V~ clippt leicht - s. README)
//   4096 = LM4040-4.096  (final, empfohlen; AC bis ~12,7 V~)
#define VREF_MV  4096
// ==========================================================
#define VREF     (VREF_MV / 1000.0f)   // -> 2.5f bzw. 4.096f
#define ADC_MAX     1023.0f
#define DC_DIVIDER  (1100.0f/100.0f) // 1M/100k ~ 1:11  (Bereich 0-30 V)
#define AC_DIVIDER  (620.0f/150.0f)  // 470k/150k (Messbereich bis ~12-13 V~)

// Nullunterdrueckung: Betraege unter dieser Schwelle werden als 0,00 angezeigt
#define DC_DEADBAND 0.20f
#define AC_DEADBAND 0.10f

// 0-V-Kanaele ausblenden (1) oder immer "0.00V" zeigen (0)
#define HIDE_ZERO   1
#define FORMFAKTOR  0.9f             // Sinus: Mittelwert(|.|) = 0.9 * Vrms

// ---- Kalibrierung  (Vwert = A * ADC + B) ----------------------------
//  Startwerte = nominal. Feinabgleich gegen Multimeter:
//  A = (U2 - U1) / (adc2 - adc1) ,  B = U1 - A * adc1   (2-Punkt)
float DC_A[6] = { VREF/ADC_MAX*DC_DIVIDER, VREF/ADC_MAX*DC_DIVIDER,
                  VREF/ADC_MAX*DC_DIVIDER, VREF/ADC_MAX*DC_DIVIDER,
                  VREF/ADC_MAX*DC_DIVIDER, VREF/ADC_MAX*DC_DIVIDER };
float DC_B[6] = { 0,0,0,0,0,0 };

//  AC: Mittelwert -> Vrms. Nominal-Gain, B faengt den Bruecken-Abfall (~1,2 V) auf.
float AC_A = VREF/ADC_MAX * AC_DIVIDER / FORMFAKTOR;   // ~0.0184 V/Count
float AC_B = 0.0f;

// ---- EEPROM-Datenstrukturen (muessen VOR der ersten Funktion stehen,
//      sonst meckert die Arduino-IDE beim Auto-Prototyp) -------------
#define EE_ADDR   0
#define EE_MAGIC  0xC643            // Kennung fuer gueltige Kalibrierdaten (pro Kanal A/B)
struct __attribute__((packed)) CalData {
  uint16_t magic; float dcA[6]; float dcB[6]; float acA; float acB; uint16_t crc;
};

#define EE_PCT_ADDR  120            // NEU V1.2: zuletzt gewaehlte Toleranz (2 Byte)
#define EE_LIM_ADDR   64            // hinter den Kalibrierdaten (die belegen 60 Byte)
#define EE_LIM_MAGIC  0x4C31        // Kennung fuer gueltige Grenzwerte
struct __attribute__((packed)) LimData {
  uint16_t magic; float lo[N_CH]; float hi[N_CH]; uint16_t crc;
};

// ---- Laufzeit-Zustand -----------------------------------------------
float val[N_CH];                    // geglaettete Anzeigewerte ([6] = AC)
float LIM_LO[N_CH], LIM_HI[N_CH];   // hi <= lo  ->  Kanal ohne Grenzen
bool  chAlarm[N_CH];                // Kanal ist (bestaetigt) ausserhalb
bool  outFlag[N_CH];                // Kanal ist gerade ausserhalb (noch unbestaetigt)
unsigned long outT[N_CH];           // seit wann ausserhalb
bool  alarmActive = false;
bool  muted = false;                // Ton/Blinken quittiert
uint8_t limPct = (uint8_t)LIM_TOL_PCT;   // NEU V1.2: Toleranz beim Einlernen (%)
bool  setupMode = false;            // NEU V1.2: Toleranz wird gerade eingestellt
unsigned long setupT = 0;           // Zeitpunkt des letzten Tippers
bool  blinkOff = false;             // NEU V1.2: Alarmkanal gerade ausgeblendet
unsigned long blinkT = 0;
bool  rawMode = false;              // Rohwerte seriell ausgeben
bool  logMode = false;              // CSV-Aufzeichnung seriell ausgeben
unsigned long lastLog = 0;

volatile bool btnHeld = false, btnEvent = false;   // aus dem Pin-Change-Interrupt
volatile unsigned long btnT0 = 0, btnDur = 0;

bool buzOn = false;
unsigned long buzEnd = 0;

// =====================================================================
void setMux(uint8_t ch) {
  digitalWrite(MUX_A, ch & 0x01);
  digitalWrite(MUX_B, (ch >> 1) & 0x01);
  digitalWrite(MUX_C, (ch >> 2) & 0x01);
  delayMicroseconds(1000);         // Mux/Puffer einschwingen (reichlich bemessen)
  (void)analogRead(ADC_PIN);       // 2 Dummy-Wandlungen verwerfen
  (void)analogRead(ADC_PIN);
}

// Mittelwert von n ADC-Messungen (0..1023, als float)
float readAvg(uint16_t n) {
  uint32_t sum = 0;
  for (uint16_t i = 0; i < n; i++) sum += analogRead(ADC_PIN);
  return (float)sum / n;
}

float readDC(uint8_t index) {         // 0..5
  setMux(index);
  float adc = readAvg(N_DC);
  float v = DC_A[index] * adc + DC_B[index];
  return (v < DC_DEADBAND) ? 0.0f : v;   // Nullunterdrueckung
}

float readACrms() {
  setMux(CH_AC);
  float mean = readAvg(N_AC);         // Mittelwert des gleichgerichteten Signals
  float v = AC_A * mean + AC_B;
  return (v < AC_DEADBAND) ? 0.0f : v;   // Nullunterdrueckung
}

float rawChannel(uint8_t ch, uint16_t n) { setMux(ch); return readAvg(n); }

// =====================================================================
//  Summer: Timer1 im CTC-Modus, der Interrupt toggelt PB6 und PB7
//  gegenphasig -> doppelte Spannung am Piezo, ohne Transistor.
// =====================================================================
ISR(TIMER1_COMPA_vect) {
  PINB = _BV(PB6) | _BV(PB7);        // Schreiben auf PINx toggelt die Pins (atomar)
}

// NEU V1.2: Jumper an J17 gesteckt -> Summer bleibt stumm (Licht blinkt weiter)
bool buzMutedByJumper() { return !(PINB & _BV(PB4)); }

void buzInit() {
  DDRB  &= ~_BV(PB4);  PORTB |=  _BV(PB4);   // MISO: Eingang mit Pull-up
  DDRB  |=  _BV(PB5);  PORTB &= ~_BV(PB5);   // SCK : Ausgang fest Low
  DDRB  |=  _BV(PB6) | _BV(PB7);
  PORTB &= ~(_BV(PB6) | _BV(PB7));
  TIMSK1 = 0;
  TCCR1A = 0;
  TCCR1B = _BV(WGM12) | _BV(CS11);   // CTC, Prescaler 8
  OCR1A  = (uint16_t)(F_CPU / 8UL / (2UL * BUZ_FREQ) - 1);
}

void beep(uint16_t ms) {
  if (buzMutedByJumper()) return;         // NEU V1.2: Jumper = kein Ton
  uint8_t s = SREG; cli();
  PORTB = (PORTB & ~_BV(PB7)) | _BV(PB6);   // Startzustand gegenphasig
  TCNT1  = 0;
  TIFR1  = _BV(OCF1A);
  TIMSK1 = _BV(OCIE1A);
  SREG = s;
  buzEnd = millis() + ms;
  buzOn  = true;
}

void buzStop() {
  uint8_t s = SREG; cli();
  TIMSK1 = 0;
  PORTB &= ~(_BV(PB6) | _BV(PB7));   // beide low -> keine Gleichspannung am Piezo
  SREG = s;
  buzOn = false;
}

void buzTask() {
  if (buzOn && (long)(millis() - buzEnd) >= 0) buzStop();
}

// Warten, ohne den Summer zu vergessen (statt delay(), wenn gerade ein Piep laeuft)
void waitMs(uint16_t ms) {
  unsigned long t = millis();
  while (millis() - t < ms) buzTask();
}

// =====================================================================
//  Taster im Betrieb: Pin-Change-Interrupt an PC3 misst die Haltedauer,
//  damit auch kurze Tipper waehrend einer Messung nicht verloren gehen.
// =====================================================================
ISR(PCINT1_vect) {
  bool down = !(PINC & _BV(PC3));
  unsigned long now = millis();
  if (down && !btnHeld) { btnHeld = true; btnT0 = now; }
  else if (!down && btnHeld) {
    unsigned long d = now - btnT0;
    btnHeld = false;
    if (d >= 40) { btnDur = d; btnEvent = true; }   // Prellen ignorieren
  }
}

void btnInit() {
  PCMSK1 |= _BV(PCINT11);            // PC3
  PCIFR   = _BV(PCIF1);
  PCICR  |= _BV(PCIE1);
}

unsigned long heldFor() {            // wie lange wird gerade gedrueckt? (0 = gar nicht)
  uint8_t s = SREG; cli();
  bool h = btnHeld; unsigned long t = btnT0;
  SREG = s;
  return h ? (millis() - t) : 0;
}

bool takeBtnEvent(unsigned long* dur) {
  uint8_t s = SREG; cli();
  bool e = btnEvent; *dur = btnDur; btnEvent = false;
  SREG = s;
  return e;
}

// =====================================================================
//  Grenzwerte: laden, speichern, einlernen, pruefen
// =====================================================================
uint16_t limCrc(const LimData& d) {
  const uint8_t* p = (const uint8_t*)&d;
  uint16_t s = 0;
  for (uint16_t i = 0; i < sizeof(LimData) - sizeof(d.crc); i++) s += p[i];
  return s ^ 0x5A5A;
}

void clearLimArrays() {
  for (uint8_t i = 0; i < N_CH; i++) { LIM_LO[i] = 0; LIM_HI[i] = 0; }
}

void loadLim() {
  LimData d; EEPROM.get(EE_LIM_ADDR, d);
  if (d.magic == EE_LIM_MAGIC && d.crc == limCrc(d)) {
    for (uint8_t i = 0; i < N_CH; i++) { LIM_LO[i] = d.lo[i]; LIM_HI[i] = d.hi[i]; }
  } else clearLimArrays();
}

void saveLim() {
  LimData d; d.magic = EE_LIM_MAGIC;
  for (uint8_t i = 0; i < N_CH; i++) { d.lo[i] = LIM_LO[i]; d.hi[i] = LIM_HI[i]; }
  d.crc = limCrc(d);
  EEPROM.put(EE_LIM_ADDR, d);
}

// NEU V1.2: zuletzt gewaehlte Toleranz im EEPROM (1 Byte + Pruefbyte)
void loadPct() {
  uint8_t p = EEPROM.read(EE_PCT_ADDR), c = EEPROM.read(EE_PCT_ADDR + 1);
  if ((uint8_t)(p ^ 0xA5) == c && p >= LIM_PCT_MIN && p <= LIM_PCT_MAX) limPct = p;
}

void savePct() {
  EEPROM.update(EE_PCT_ADDR, limPct);
  EEPROM.update(EE_PCT_ADDR + 1, (uint8_t)(limPct ^ 0xA5));
}

bool hasLim(uint8_t i) { return LIM_HI[i] > LIM_LO[i]; }

bool anyLim() {
  for (uint8_t i = 0; i < N_CH; i++) if (hasLim(i)) return true;
  return false;
}

void resetAlarmState() {
  for (uint8_t i = 0; i < N_CH; i++) { chAlarm[i] = false; outFlag[i] = false; }
  alarmActive = false; muted = false;
}

// Jeder aktive Kanal bekommt seinen aktuellen Wert +- Toleranz, inaktive
// Kanaele werden freigegeben. Rueckgabe: Anzahl eingelernter Kanaele.
uint8_t learnLimits() {
  uint8_t n = 0;
  for (uint8_t i = 0; i < N_CH; i++) {
    if (val[i] > 0.0f) {
      float w = val[i] * (limPct / 100.0f);
      if (w < LIM_MIN_ABS) w = LIM_MIN_ABS;
      LIM_LO[i] = val[i] - w;
      LIM_HI[i] = val[i] + w;
      n++;
    } else { LIM_LO[i] = 0; LIM_HI[i] = 0; }
  }
  saveLim();
  resetAlarmState();
  return n;
}

void clearLimits() {
  clearLimArrays();
  saveLim();
  resetAlarmState();
}

// Alarm nur, wenn das Geraet ueberhaupt laeuft: sind ALLE ueberwachten
// Kanaele bei 0 V, ist der Pruefling aus -> kein Alarm. Faellt dagegen nur
// ein Teil der Schienen weg, ist genau das der Fehler, den man sucht.
void evalAlarm() {
  bool live = false;
  for (uint8_t i = 0; i < N_CH; i++) if (hasLim(i) && val[i] > 0.0f) live = true;

  unsigned long now = millis();
  bool any = false;
  for (uint8_t i = 0; i < N_CH; i++) {
    bool out = live && hasLim(i) && (val[i] < LIM_LO[i] || val[i] > LIM_HI[i]);
    if (out) {
      if (!outFlag[i]) { outFlag[i] = true; outT[i] = now; }
      chAlarm[i] = (now - outT[i]) >= ALARM_DELAY_MS;
    } else {
      outFlag[i] = false; chAlarm[i] = false;
    }
    if (chAlarm[i]) any = true;
  }
  if (!any) muted = false;           // Alarm vorbei -> naechster Alarm meldet sich wieder
  alarmActive = any;
}

// =====================================================================
//  Anzeige
// =====================================================================
void printVal(uint8_t col, uint8_t row, char nr, float v, bool alarm, bool hide) {
  lcd.setCursor(col, row);
  lcd.print(nr);
  lcd.print(alarm ? '!' : ':');        // "!" markiert einen Kanal ausserhalb der Grenzen
  if (hide) { lcd.print("ALARM "); return; }   // NEU V1.2: Blinkphase (6 Zeichen)
#if HIDE_ZERO
  if (v <= 0.0f && !alarm) { lcd.print("      "); return; }   // Wert ausblenden (6 Leerz.)
#endif
  char buf[10];
  dtostrf(v, 5, 2, buf);              // 5 Zeichen, rechtsbuendig: "12.34" / " 9.87"
  lcd.print(buf); lcd.print("V");
}

// NEU V1.2: AC-Feld (Zeile 4, Spalte 0..13)
void printAC(bool hide) {
  lcd.setCursor(0, 3);
  lcd.print(chAlarm[CH_AC] ? "AC!" : "AC:");
  if (hide) { lcd.print("ALARM      "); return; }     // 11 Zeichen
  float ac = val[CH_AC];
#if HIDE_ZERO
  if (ac <= 0.0f && !chAlarm[CH_AC]) { lcd.print("           "); return; }
#endif
  char buf[10];
  dtostrf(ac, 5, 2, buf);
  lcd.print(buf); lcd.print(" Vrms ");
}

// NEU V1.2: einen einzelnen Kanal neu zeichnen (fuer das Blinken)
void drawChannel(uint8_t i, bool hide) {
  if (i == CH_AC) { printAC(hide); return; }
  const uint8_t col = (i < 3) ? 0 : 12;
  const uint8_t row = (i < 3) ? i : (i - 3);
  printVal(col, row, (char)('1' + i), val[i], chAlarm[i], hide);
}

// NEU V1.2: Der Kanal mit Alarm blinkt im Display zwischen Messwert und
// "ALARM" (500 ms), damit man ihn aus der Entfernung sieht - das "!" allein
// faellt kaum auf. Blinkt unabhaengig vom Hintergrundlicht.
void blinkTask() {
  bool active = alarmActive && !muted;
  if (active) {
    if (millis() - blinkT < BLINK_MS) return;
    blinkT = millis();
    blinkOff = !blinkOff;
  } else {
    if (!blinkOff) return;                  // nichts ausgeblendet -> nichts zu tun
    blinkOff = false;                       // Alarm vorbei/stumm -> Werte zurueck
  }
  for (uint8_t i = 0; i < N_CH; i++)
    if (chAlarm[i]) drawChannel(i, blinkOff);
}

// Statusfeld unten rechts (Spalte 14..19, 6 Zeichen)
void drawStatus() {
  char buf[8];
  const char* t = "      ";
  unsigned long h = heldFor();
  if      (h >= HOLD_CLEAR_MS)       t = "LOESCH";
  else if (h >= HOLD_LEARN_MS)       t = "SETUP ";
  else if (setupMode) {                                  // NEU V1.2
    snprintf(buf, sizeof(buf), "+-%2u%% ", (unsigned)(limPct > 99 ? 99 : limPct));
    t = buf;
  }
  else if (alarmActive && muted)     t = "STUMM ";
  else if (alarmActive)              t = "ALARM!";
  else if (anyLim())                 t = "UEBERW";
  lcd.setCursor(14, 3);
  lcd.print(t);
}

void updateLCD() {
  //  20x4-Layout:  1:xx.xxV    4:xx.xxV
  //                2:xx.xxV    5:xx.xxV
  //                3:xx.xxV    6:xx.xxV
  //                AC:xx.xx Vrms ALARM!
  bool h0 = blinkOff && chAlarm[0], h1 = blinkOff && chAlarm[1], h2 = blinkOff && chAlarm[2];
  bool h3 = blinkOff && chAlarm[3], h4 = blinkOff && chAlarm[4], h5 = blinkOff && chAlarm[5];
  printVal(0, 0, '1', val[0], chAlarm[0], h0);  lcd.print("    ");  printVal(12, 0, '4', val[3], chAlarm[3], h3);
  printVal(0, 1, '2', val[1], chAlarm[1], h1);  lcd.print("    ");  printVal(12, 1, '5', val[4], chAlarm[4], h4);
  printVal(0, 2, '3', val[2], chAlarm[2], h2);  lcd.print("    ");  printVal(12, 2, '6', val[5], chAlarm[5], h5);

  printAC(blinkOff && chAlarm[CH_AC]);
  drawStatus();
}

void showMessage(const char* l1, const char* l2, uint16_t ms) {
  lcd.clear();
  lcd.setCursor(0, 1); lcd.print(l1);
  lcd.setCursor(0, 2); lcd.print(l2);
  waitMs(ms);
  lcd.clear();
}

// Licht und Ton je nach Alarmzustand; wird einmal pro Messrunde aufgerufen
void alarmOutputs() {
  static bool phase = false;
  if (alarmActive && !muted) {
    phase = !phase;
    analogWrite(BL_PIN, phase ? BL_BRIGHT : BL_ALARM_LOW);
    if (phase) beep(120);
  } else {
    phase = false;
    analogWrite(BL_PIN, BL_BRIGHT);
  }
}

// =====================================================================
//  Taster-Ereignisse im Betrieb
// =====================================================================
void handleButton() {
  unsigned long d;
  if (!takeBtnEvent(&d)) return;

  if (d >= HOLD_CLEAR_MS) {
    setupMode = false;
    clearLimits();
    buzStop(); beep(400);
    Serial.println(F("Grenzen geloescht."));
    showMessage("Grenzen geloescht", "Ueberwachung aus", 1500);
  } else if (d >= HOLD_LEARN_MS) {          // NEU V1.2: Einrichten starten
    setupMode = true;
    limPct = LIM_PCT_MIN;
    setupT = millis();
    buzStop(); beep(40);
    Serial.println(F("Grenzen einrichten: tippen = +1 %, 5 s warten = einlernen"));
  } else {                                  // kurz getippt
    if (setupMode) {                        // NEU V1.2: Toleranz hochzaehlen
      limPct++;
      if (limPct > LIM_PCT_MAX) limPct = LIM_PCT_MIN;
      setupT = millis();
      buzStop(); beep(20);
      Serial.print(F("Toleranz +-")); Serial.print(limPct); Serial.println(F(" %"));
    } else if (alarmActive) {
      muted = true; buzStop(); analogWrite(BL_PIN, BL_BRIGHT);
    }
  }
}

// NEU V1.2: 5 s kein Tipper mehr -> mit der gezeigten Toleranz einlernen
void setupTask() {
  if (!setupMode || (millis() - setupT) < SETUP_IDLE_MS) return;
  setupMode = false;
  savePct();
  uint8_t n = learnLimits();
  buzStop(); beep(80);
  Serial.print(F("Grenzen eingelernt (+-")); Serial.print(limPct);
  Serial.print(F(" %): ")); Serial.print(n); Serial.println(F(" Kanal/Kanaele"));
  if (n == 0) showMessage("Nichts eingelernt:", "keine Spannung da", 1800);
  else {
    char l2[21];
    snprintf(l2, sizeof(l2), "%u Kanal(e) +-%u%%", n, limPct);
    showMessage("Grenzen eingelernt", l2, 1500);
  }
}

// =====================================================================
//  Serielle Konsole (J15, 9600 Baud, Zeilenende "Neue Zeile")
// =====================================================================
float readFloatBlocking() {          // wartet auf eine nicht-leere Eingabezeile
  for (;;) {
    while (!Serial.available()) { }
    String s = Serial.readStringUntil('\n');
    s.trim();
    s.replace(',', '.');             // "12,5" wird wie "12.5" verstanden
    if (s.length()) return s.toFloat();
  }
}

void printHelp() {
  Serial.println(F("--- Kommandos ---"));
  Serial.println(F("?      Hilfe"));
  Serial.println(F("l      Datenaufzeichnung (CSV) an/aus"));
  Serial.println(F("r      Rohwerte (ADC-Counts) an/aus"));
  Serial.println(F("s      alle laufenden Ausgaben stoppen"));
  Serial.println(F("1..6   DC-Kanal 1..6 kalibrieren (2-Punkt)"));
  Serial.println(F("a      AC-Kanal kalibrieren (2-Punkt)"));
  Serial.println(F("g      Grenzwerte anzeigen"));
  Serial.println(F("G      Grenzwert fuer einen Kanal setzen"));
  Serial.println(F("e      Grenzen einlernen (mit der aktuellen Toleranz)"));
  Serial.println(F("t      Toleranz fuer das Einlernen setzen (1..20 %)"));
  Serial.println(F("x      alle Grenzen loeschen (wie Taste >= 6 s)"));
  Serial.println(F("m      Alarmton stumm (wie Taste kurz)"));
  Serial.println(F("b      Summer-Test"));
  Serial.print(F("Toleranz: +-")); Serial.print(limPct);
  Serial.print(F(" %   Summer: "));
  Serial.print(buzMutedByJumper() ? F("per Jumper AUS") : F("an"));
  Serial.print(F("   (PB4=")); Serial.print((PINB & _BV(PB4)) ? '1' : '0');
  Serial.println(F(")"));
}

void printChName(uint8_t i) {
  if (i == CH_AC) Serial.print(F("AC ")); else { Serial.print(F("DC")); Serial.print(i + 1); }
}

void printLimits() {
  Serial.print(F("--- Grenzwerte (Toleranz beim Einlernen: +-"));
  Serial.print(limPct); Serial.println(F(" %) ---"));
  for (uint8_t i = 0; i < N_CH; i++) {
    printChName(i); Serial.print(F(": "));
    if (hasLim(i)) {
      Serial.print(LIM_LO[i], 2); Serial.print(F(" .. ")); Serial.print(LIM_HI[i], 2);
      Serial.print(F(" V   aktuell ")); Serial.print(val[i], 2);
      if (chAlarm[i]) Serial.print(F("  ALARM"));
      Serial.println();
    } else Serial.println(F("nicht ueberwacht"));
  }
}

void setLimitInteractive() {
  buzStop();
  Serial.println(F("Kanal (1..6 = DC1..DC6, 7 = AC):"));
  int ch = (int)readFloatBlocking() - 1;
  if (ch < 0 || ch >= N_CH) { Serial.println(F("ungueltiger Kanal - abgebrochen.")); return; }
  Serial.println(F("untere Grenze in V (0 und 0 = Ueberwachung aus):"));
  float lo = readFloatBlocking();
  Serial.println(F("obere Grenze in V:"));
  float hi = readFloatBlocking();
  if (hi <= lo) { LIM_LO[ch] = 0; LIM_HI[ch] = 0; }
  else          { LIM_LO[ch] = lo; LIM_HI[ch] = hi; }
  saveLim();
  resetAlarmState();
  printChName(ch);
  if (hasLim(ch)) { Serial.print(F(" ueberwacht: ")); Serial.print(lo, 2); Serial.print(F(" .. ")); Serial.print(hi, 2); Serial.println(F(" V")); }
  else Serial.println(F(" nicht mehr ueberwacht."));
}

void calibrate(uint8_t ch) {         // ch 0..5 = DC1..DC6, 6 = AC
  buzStop();
  bool isAC = (ch == CH_AC);
  uint16_t n = isAC ? N_AC : N_DC;
  Serial.println();
  Serial.print(F("=== Kalibrierung "));
  if (isAC) Serial.println(F("AC ==="));
  else { Serial.print(F("DC")); Serial.print(ch + 1); Serial.println(F(" ===")); }

  Serial.println(F("Punkt 1: bekannte Spannung anlegen, Wert in V eingeben + Enter:"));
  float v1 = readFloatBlocking();
  float c1 = rawChannel(ch, n);
  Serial.print(F("  Rohwert 1 = ")); Serial.println(c1, 1);

  Serial.println(F("Punkt 2: andere bekannte Spannung anlegen, Wert in V eingeben + Enter:"));
  float v2 = readFloatBlocking();
  float c2 = rawChannel(ch, n);
  Serial.print(F("  Rohwert 2 = ")); Serial.println(c2, 1);

  if (fabs(c2 - c1) < 1.0f) { Serial.println(F("Rohwerte zu dicht beisammen - abgebrochen.")); return; }
  float A = (v2 - v1) / (c2 - c1);
  float B = v1 - A * c1;
  if (isAC) { AC_A = A; AC_B = B; } else { DC_A[ch] = A; DC_B[ch] = B; }

  Serial.println(F("--- Ergebnis (in den Sketch uebernehmen) ---"));
  if (isAC) {
    Serial.print(F("AC_A = ")); Serial.print(A, 6); Serial.println(F("f;"));
    Serial.print(F("AC_B = ")); Serial.print(B, 4); Serial.println(F("f;"));
  } else {
    Serial.print(F("DC_A[")); Serial.print(ch); Serial.print(F("] = ")); Serial.print(A, 6); Serial.println(F("f;"));
    Serial.print(F("DC_B[")); Serial.print(ch); Serial.print(F("] = ")); Serial.print(B, 4); Serial.println(F("f;"));
  }
  Serial.println(F("(bereits aktiv fuer diese Sitzung)"));
}

void logHeader() {
  Serial.print(F("Zeit_s"));
  for (uint8_t i = 0; i < 6; i++) { Serial.print(LOG_SEP); Serial.print(F("DC")); Serial.print(i + 1); }
  Serial.print(LOG_SEP); Serial.print(F("AC_Vrms"));
  Serial.print(LOG_SEP); Serial.println(F("Alarm"));
}

void logNum(float v, uint8_t digits) {
  char buf[12];
  dtostrf(v, 1, digits, buf);
#if LOG_KOMMA
  for (char* p = buf; *p; p++) if (*p == '.') *p = ',';
#endif
  Serial.print(buf);
}

void logLine() {
  logNum(millis() / 1000.0f, 1);
  for (uint8_t i = 0; i < N_CH; i++) { Serial.print(LOG_SEP); logNum(val[i], 2); }
  Serial.print(LOG_SEP);
  bool first = true;                  // Kanaele im Alarm, z. B. "DC2 AC"
  for (uint8_t i = 0; i < N_CH; i++) if (chAlarm[i]) {
    if (!first) Serial.print(' ');
    if (i == CH_AC) Serial.print(F("AC")); else { Serial.print(F("DC")); Serial.print(i + 1); }
    first = false;
  }
  Serial.println();
}

void handleSerial() {
  if (!Serial.available()) return;
  int cmd = Serial.read();
  switch (cmd) {
    case '?': printHelp(); break;
    case 'l':
      logMode = !logMode; rawMode = false;
      if (logMode) { logHeader(); lastLog = 0; }
      else Serial.println(F("Aufzeichnung AUS"));
      break;
    case 'v': if (!logMode) { logMode = true; rawMode = false; logHeader(); lastLog = 0; } break;  // alt
    case 'r':
      rawMode = !rawMode; logMode = false;
      Serial.println(rawMode ? F("Roh-Modus AN") : F("Roh-Modus AUS"));
      break;
    case 's': rawMode = false; logMode = false; Serial.println(F("Ausgabe gestoppt")); break;
    case 'a': calibrate(CH_AC); break;
    case '1': case '2': case '3':
    case '4': case '5': case '6': calibrate(cmd - '1'); break;
    case 'g': printLimits(); break;
    case 'G': setLimitInteractive(); break;
    case 'e': {
      uint8_t n = learnLimits();
      Serial.print(F("Grenzen eingelernt: ")); Serial.print(n); Serial.println(F(" Kanal/Kanaele"));
      printLimits();
      break;
    }
    case 't': {                        // NEU V1.2: Toleranz setzen
      Serial.println(F("Toleranz in % (1..20):"));
      int p = (int)readFloatBlocking();
      if (p < LIM_PCT_MIN || p > LIM_PCT_MAX) { Serial.println(F("ausserhalb 1..20 - abgebrochen.")); break; }
      limPct = (uint8_t)p; savePct();
      Serial.print(F("Toleranz jetzt +-")); Serial.print(limPct); Serial.println(F(" % (gespeichert)"));
      break;
    }
    case 'x': clearLimits(); Serial.println(F("Grenzen geloescht.")); break;
    case 'm': if (alarmActive) { muted = true; buzStop(); } Serial.println(F("Alarmton stumm")); break;
    case 'b':
      if (buzMutedByJumper()) Serial.println(F("Summer ist per Jumper (J18, Pin 1-3) stumm."));
      else beep(300);
      break;
    default: break;                   // \r, \n und Unbekanntes ignorieren
  }
}

// =====================================================================
//  Kalibrierung per Taster + EEPROM  (pro Kanal eigenes A/B)
//  Taster an CAL_BUTTON gegen GND (interner Pull-up).
//  Beim EINSCHALTEN Taster gedrueckt halten -> Kalibriermodus.
//  Ablauf: erst DC1..DC6 mit 5,00 V, dann DC1..DC6 mit 12,00 V.
//    Jeweils Spannung anlegen und Taste TIPPEN. Erkennt das Geraet an einem
//    Punkt KEINE (oder eine stark abweichende) Spannung, wird er automatisch
//    uebersprungen -> der Kanal behaelt seinen bisherigen Wert. So sind auch
//    einzelne Kanaele nachkalibrierbar. Alle Werte werden im EEPROM abgelegt.
// =====================================================================
uint16_t calCrc(const CalData& d) {
  const uint8_t* p = (const uint8_t*)&d;
  uint16_t s = 0;
  for (uint16_t i = 0; i < sizeof(CalData) - sizeof(d.crc); i++) s += p[i];
  return s ^ 0xA5A5;
}

void loadCal() {                     // beim Start: gespeicherte Werte holen
  CalData d; EEPROM.get(EE_ADDR, d);
  if (d.magic == EE_MAGIC && d.crc == calCrc(d)) {
    for (uint8_t i = 0; i < 6; i++) { DC_A[i] = d.dcA[i]; DC_B[i] = d.dcB[i]; }
    AC_A = d.acA; AC_B = d.acB;
  }                                  // sonst: Nominalwerte behalten
}

void saveCalAll() {                  // alle aktuellen A/B ins EEPROM
  CalData d; d.magic = EE_MAGIC;
  for (uint8_t i = 0; i < 6; i++) { d.dcA[i] = DC_A[i]; d.dcB[i] = DC_B[i]; }
  d.acA = AC_A; d.acB = AC_B; d.crc = calCrc(d);
  EEPROM.put(EE_ADDR, d);
}

bool btnDown() { return digitalRead(CAL_BUTTON) == LOW; }

void waitTap() {                     // ein entprellter Tastendruck
  while (!btnDown()) { }
  delay(30);
  while ( btnDown()) { }
  delay(30);
}

void calStep(uint8_t chNum, uint8_t volt, float* raw, bool* got) {
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("KALIBRIERUNG");
  lcd.setCursor(0,1); lcd.print("DC"); lcd.print((int)chNum);
  lcd.print(": "); lcd.print((int)volt); lcd.print(".00 V");
  lcd.setCursor(0,2); lcd.print("Spannung anlegen,");
  lcd.setCursor(0,3); lcd.print("dann Taste tippen");
  waitTap();
  float r = rawChannel(chNum - 1, N_DC);
  float vnom = r * (VREF / ADC_MAX * DC_DIVIDER);   // grobe Spannung (nominal)
  if (vnom > volt * 0.8f && vnom < volt * 1.2f) {   // plausibel (+-20%) -> uebernehmen
    *raw = r; *got = true;
    beep(60);
    lcd.setCursor(0,2); lcd.print("gemessen        ");
    lcd.setCursor(0,3); lcd.print("                  ");
  } else {                                          // keine/falsche Spannung
    *got = false;
    lcd.setCursor(0,2); lcd.print("uebersprungen   ");
    lcd.setCursor(0,3); lcd.print("(keine Spannung)  ");
  }
  waitMs(900);
}

void runCalibration() {
  float raw5[6], raw12[6];
  bool  got5[6], got12[6];
  for (uint8_t i = 0; i < 6; i++) { got5[i] = false; got12[i] = false; }

  lcd.clear();
  lcd.setCursor(0,0); lcd.print("KALIBRIERUNG");
  lcd.setCursor(0,1); lcd.print("Spannung anlegen +");
  lcd.setCursor(0,2); lcd.print("tippen. Ohne Spg.");
  lcd.setCursor(0,3); lcd.print("wird uebersprungen");
  delay(2800);

  for (uint8_t ch = 1; ch <= 6; ch++) calStep(ch,  5, &raw5[ch-1],  &got5[ch-1]);
  for (uint8_t ch = 1; ch <= 6; ch++) {
    if (!got5[ch-1]) { got12[ch-1] = false; continue; }  // bei 5V uebersprungen -> auch hier
    calStep(ch, 12, &raw12[ch-1], &got12[ch-1]);
  }

  uint8_t n = 0;
  for (uint8_t i = 0; i < 6; i++) {
    if (got5[i] && got12[i] && fabs(raw12[i] - raw5[i]) > 2.0f) {
      float A = (12.0f - 5.0f) / (raw12[i] - raw5[i]);
      DC_A[i] = A;
      DC_B[i] = 5.0f - A * raw5[i];
      n++;
    }
  }
  saveCalAll();

  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Gespeichert!");
  lcd.setCursor(0,1); lcd.print((int)n); lcd.print(" Kanal(e) neu");
  if (n == 0) {                      // nichts kalibriert -> Grund anzeigen
    lcd.setCursor(0,2); lcd.print("je 5V UND 12V");
    lcd.setCursor(0,3); lcd.print("pro Kanal noetig!");
  }
  delay(3500);
  lcd.clear();                       // Reste vor der Messanzeige entfernen
}

// =====================================================================
void setup() {
  pinMode(BL_PIN, OUTPUT);
  analogWrite(BL_PIN, BL_BRIGHT);    // Licht sofort an (neue Platine: sonst dunkel)
  buzInit();

  Serial.begin(9600);
  pinMode(MUX_A, OUTPUT); pinMode(MUX_B, OUTPUT); pinMode(MUX_C, OUTPUT);
  pinMode(CAL_BUTTON, INPUT_PULLUP);
  // WICHTIG: externe Referenz (LM4040) an AREF.
  //  -> EXTERNAL VOR dem ersten analogRead setzen.
  //  -> Niemals eine interne Referenz waehlen, solange AREF extern anliegt!
  analogReference(EXTERNAL);
  Serial.setTimeout(30000);          // fuer Kalibrier-Eingaben
  loadCal();                         // gespeicherte Kalibrierung laden (falls vorhanden)
  loadLim();                         // gespeicherte Grenzwerte laden (falls vorhanden)
  loadPct();                         // NEU V1.2: zuletzt gewaehlte Toleranz
  resetAlarmState();

  lcd.begin(20, 4);   // Kontrast per Trimmer R4 an V0, Licht ueber R3 und Q1
  lcd.setCursor(0,0); lcd.print("  Messsystem 1MHz.de");
  lcd.setCursor(0,1); lcd.print("  6x DC + 1x AC");
  lcd.setCursor(0,2); lcd.print("  V1.2  10.2026 @mad");
  beep(60);                          // kurzer Piep: Summer funktioniert
  waitMs(1500); lcd.clear();

  if (btnDown()) runCalibration();   // Taste beim Einschalten gedrueckt -> kalibrieren
  while (btnDown()) { }              // loslassen abwarten, sonst zaehlt es als Halten
  btnInit();                         // ab jetzt: Taster im Betrieb per Interrupt

  Serial.print(F("Messsystem V1.2 bereit. Toleranz +-")); Serial.print(limPct);
  Serial.println(F(" %. '?' + Enter fuer Kommandos."));
  if (buzMutedByJumper()) Serial.println(F("Summer per Jumper (J18, Pin 1-3) ausgeschaltet."));
}

void loop() {
  static bool inited = false;

  // ---- messen ----
  float m[N_CH];
  for (uint8_t i = 0; i < 6; i++) m[i] = readDC(i);
  m[CH_AC] = readACrms();

  // ---- glaetten; grosse Spruenge (Ein/Aus) sofort uebernehmen ----
  for (uint8_t i = 0; i < N_CH; i++) {
    float dead = (i == CH_AC) ? AC_DEADBAND : DC_DEADBAND;
    if (!inited) val[i] = m[i];
    else {
      float d = m[i] - val[i];
      val[i] = (fabs(d) > 0.5f) ? m[i] : val[i] + SMOOTH * d;
    }
    if (val[i] < dead) val[i] = 0.0f;  // unter Schwelle -> sauber 0 (ausgeblendet)
  }
  inited = true;

  // ---- auswerten, anzeigen, melden ----
  evalAlarm();
  updateLCD();
  alarmOutputs();

  if (rawMode) {                       // Rohwerte (Counts) - fuer die Kalibrierung
    for (uint8_t i = 0; i < 6; i++) { Serial.print(rawChannel(i, N_DC), 1); Serial.print('\t'); }
    Serial.print(F("ACraw=")); Serial.println(rawChannel(CH_AC, N_AC), 1);
  } else if (logMode && (lastLog == 0 || millis() - lastLog >= LOG_MS)) {
    lastLog = millis();
    logLine();
  }

  // ---- kurze Pause, dabei Taster, Konsole und Summer bedienen ----
  unsigned long t0 = millis(), tStat = 0;
  while (millis() - t0 < 200) {
    buzTask();
    handleButton();
    setupTask();
    blinkTask();
    handleSerial();
    if (millis() - tStat >= 50) { tStat = millis(); drawStatus(); }
  }
}
