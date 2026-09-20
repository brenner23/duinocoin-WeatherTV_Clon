#ifndef NM_DISPLAY_H
#define NM_DISPLAY_H

#if defined(DISPLAY_NMTV154)

#include <Arduino.h>
#include <time.h>
#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266HTTPClient.h>
  #include <WiFiClientSecureBearSSL.h>
#else
  #include <WiFi.h>
  #include <HTTPClient.h>
  #include <WiFiClientSecure.h>
#endif
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Fonts/AGENCYB15pt7b.h>
#include <Fonts/AGENCYB20pt7b.h>
#include <Fonts/AGENCYB30pt7b.h>
#include "pins.h"

// Die Instanz selbst wird in ESP_Code_NMTV154_Adafruit.ino erzeugt.
extern Adafruit_ST7789 tft;

// Wallet-Daten
float duco_balance = 0.0f;
int duco_max_miners = 0;
int duco_warnings = 0;
bool duco_verified = false;
bool duco_wallet_valid = false;
unsigned long duco_wallet_last_update = 0;

// V8: Display-Zustand absichtlich ganz oben deklariert.
// So sind die Variablen bereits fuer screen_setup(), display_boot()
// und display_info() sichtbar.
static bool nm_dashboard_drawn = false;
#if defined(ESP8266)
static bool nm_display_mutex = false;
#else
static SemaphoreHandle_t nm_display_mutex = nullptr;
#endif

#define NM_DISPLAY_BUILD "V8"

// Farben RGB565
static const uint16_t C_BG     = 0x0841;
static const uint16_t C_PANEL  = 0x10A2;
static const uint16_t C_PANEL2 = 0x18E3;
static const uint16_t C_ACCENT = 0xF5C0;
static const uint16_t C_GREEN  = 0x4E68;
static const uint16_t C_BLUE   = 0x3D9F;
static const uint16_t C_RED    = 0xF986;
static const uint16_t C_TEXT   = ST77XX_WHITE;
static const uint16_t C_MUTED  = 0xAD55;
static const uint16_t C_LINE   = 0x2945;

static void nm_text(const String &text, int16_t x, int16_t y,
                    uint8_t size, uint16_t color,
                    uint16_t bg = C_PANEL) {
  tft.setTextWrap(false);
  tft.setTextSize(size);
  tft.setTextColor(color, bg);
  tft.setCursor(x, y);
  tft.print(text);
}

static int16_t nm_text_width(const String &text, uint8_t size) {
  int16_t x1, y1;
  uint16_t w, h;
  tft.setTextSize(size);
  tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  return (int16_t)w;
}

static void nm_text_right(const String &text, int16_t right, int16_t y,
                          uint8_t size, uint16_t color,
                          uint16_t bg = C_PANEL) {
  nm_text(text, right - nm_text_width(text, size), y, size, color, bg);
}

static void nm_text_center(const String &text, int16_t centerX, int16_t y,
                           uint8_t size, uint16_t color,
                           uint16_t bg) {
  nm_text(text, centerX - nm_text_width(text, size) / 2, y, size, color, bg);
}



// Eigene GFX-Fonts aus dem Adafruit_GFX/Fonts-Ordner.
static void nm_gfx_text(const String &text, int16_t x, int16_t baseline,
                        const GFXfont *font, uint16_t color) {
  tft.setTextWrap(false);
  tft.setFont(font);
  tft.setTextSize(1);
  tft.setTextColor(color);
  tft.setCursor(x, baseline);
  tft.print(text);
  tft.setFont();
}

static int16_t nm_gfx_width(const String &text, const GFXfont *font) {
  int16_t x1, y1;
  uint16_t w, h;
  tft.setFont(font);
  tft.setTextSize(1);
  tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  tft.setFont();
  return (int16_t)w;
}

static void nm_gfx_center(const String &text, int16_t centerX, int16_t baseline,
                          const GFXfont *font, uint16_t color) {
  const int16_t w = nm_gfx_width(text, font);
  nm_gfx_text(text, centerX - w / 2, baseline, font, color);
}

// Exakten Pixelbereich eines GFX-Textes ermitteln und löschen.
// Das ist wichtig, weil GFX-Fonts transparent gezeichnet werden und
// ihre Glyphen deutlich über grobe "Textzeilen"-Rechtecke hinausragen können.
static void nm_clear_gfx_text(const String &text,
                              int16_t x, int16_t baseline,
                              const GFXfont *font,
                              uint16_t bg,
                              int16_t clipX, int16_t clipY,
                              int16_t clipW, int16_t clipH,
                              int16_t pad = 2) {
  if (text.length() == 0) return;

  int16_t x1, y1;
  uint16_t w, h;

  tft.setFont(font);
  tft.setTextSize(1);
  tft.getTextBounds(text, x, baseline, &x1, &y1, &w, &h);
  tft.setFont();

  int16_t rx = x1 - pad;
  int16_t ry = y1 - pad;
  int16_t rw = (int16_t)w + pad * 2;
  int16_t rh = (int16_t)h + pad * 2;

  // Auf den jeweiligen Kartenbereich begrenzen, damit Labels/Rahmen
  // nicht versehentlich weggeputzt werden.
  int16_t right  = ((rx + rw) < (clipX + clipW)) ? (rx + rw) : (clipX + clipW);
  int16_t bottom = ((ry + rh) < (clipY + clipH)) ? (ry + rh) : (clipY + clipH);
  rx = (rx > clipX) ? rx : clipX;
  ry = (ry > clipY) ? ry : clipY;
  rw = right - rx;
  rh = bottom - ry;

  if (rw > 0 && rh > 0) {
    tft.fillRect(rx, ry, rw, rh, bg);
  }
}

static const GFXfont* nm_share_font_for(const String &sharesText) {
  int16_t x1, y1;
  uint16_t w, h;

  tft.setFont(&AGENCYB20pt7b);
  tft.setTextSize(1);
  tft.getTextBounds(sharesText, 0, 0, &x1, &y1, &w, &h);
  tft.setFont();

  return (w <= 137) ? &AGENCYB20pt7b : &AGENCYB15pt7b;
}

static void nm_draw_wifi_bars(int16_t x, int16_t y) {
  const bool connected = (WiFi.status() == WL_CONNECTED);
  int bars = 0;

  if (connected) {
    const int rssi = WiFi.RSSI();
    if (rssi >= -55)      bars = 4;
    else if (rssi >= -67) bars = 3;
    else if (rssi >= -75) bars = 2;
    else if (rssi >= -85) bars = 1;
  }

  const uint16_t onColor  = connected ? C_GREEN : C_RED;
  const uint16_t offColor = C_LINE;
  const int16_t w = 5;
  const int16_t gap = 3;
  const int16_t heights[4] = {5, 9, 13, 17};

  // Vier klassische Handy-Empfangsbalken, von klein nach gross.
  for (int i = 0; i < 4; ++i) {
    const int16_t h = heights[i];
    const int16_t bx = x + i * (w + gap);
    const int16_t by = y + 17 - h;

    if (i < bars) {
      tft.fillRect(bx, by, w, h, onColor);
    } else {
      tft.drawRect(bx, by, w, h, offColor);
    }
  }

  // Bei komplett getrennter WLAN-Verbindung ein kleines rotes X davor.
  if (!connected) {
    tft.drawLine(x - 9, y + 4, x - 3, y + 10, C_RED);
    tft.drawLine(x - 3, y + 4, x - 9, y + 10, C_RED);
  }
}

static String nm_short_node(String node) {
  node.trim();
  if (node.length() > 12) return node.substring(0, 12);
  return node;
}

static String nm_format_uptime(unsigned long ms) {
  unsigned long sec = ms / 1000UL;
  unsigned int days = sec / 86400UL;
  unsigned int hours = (sec / 3600UL) % 24UL;
  unsigned int mins = (sec / 60UL) % 60UL;

  if (days > 0) return String(days) + "d " + String(hours) + "h";
  return String(hours) + "h " + String(mins) + "m";
}


static void nm_display_power_on() {
  // WAthER TV: kein separater Display-Enable-Pin. Backlight ist LOW-aktiv.
  if (!nm_backlight_pwm_active) {
    pinMode(TFT_BACKLIGHT, OUTPUT);
    digitalWrite(TFT_BACKLIGHT, BL_ON);
  }
}

void screen_setup() {
  nm_display_power_on();

#if !defined(ESP8266)
  if (nm_display_mutex == nullptr) {
    nm_display_mutex = xSemaphoreCreateMutex();
  }
#endif

  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);

#if defined(ESP8266)
  // Bewaehrte WAthER-TV-Initialisierung aus dem vorhandenen Projekt.
  SPI.begin();
  tft.init(240, 240, SPI_MODE3);
#else
  SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
  tft.init(SCREEN_W, SCREEN_H);
#endif

  tft.setRotation(2);
  tft.setTextWrap(false);
  tft.fillScreen(C_BG);
}

void display_boot() {
  nm_dashboard_drawn = false;
  nm_display_power_on();
  tft.fillScreen(C_BG);

  tft.fillRoundRect(10, 12, 220, 56, 8, C_ACCENT);
  nm_text_center("DUINO-COIN", 120, 23, 3, ST77XX_BLACK, C_ACCENT);
  nm_text_center("NM-TV-154", 120, 50, 1, ST77XX_BLACK, C_ACCENT);

  nm_text_center("Miner startet ...", 120, 102, 2, C_TEXT, C_BG);
  nm_text_center(String(RIG_IDENTIFIER), 120, 132, 1, C_MUTED, C_BG);
  nm_text_center("ESP8266  |  v" + String(SOFTWARE_VERSION), 120, 154, 1, C_MUTED, C_BG);

  tft.drawRoundRect(30, 188, 180, 8, 4, C_LINE);
  tft.fillRoundRect(32, 190, 105, 4, 2, C_GREEN);
  nm_text_center("WLAN + Pool werden verbunden", 120, 214, 1, C_MUTED, C_BG);
}

void display_info(String message) {
  nm_dashboard_drawn = false;
  nm_display_power_on();
  tft.fillRect(0, 182, 240, 58, C_BG);
  tft.fillRoundRect(10, 190, 220, 40, 7, C_PANEL);
  if (message.length() > 30) message = message.substring(0, 30);
  nm_text_center(message, 120, 204, 1, C_ACCENT, C_PANEL);
}

bool update_wallet_data() {
  if (WiFi.status() != WL_CONNECTED) {
    #if defined(SERIAL_PRINTING)
      Serial.println("[WALLET] Kein WLAN - Abfrage uebersprungen");
    #endif
    return false;
  }

  #if defined(SERIAL_PRINTING)
    Serial.println();
    Serial.println("[WALLET] Anfrage startet...");
    Serial.println("[WALLET] URL: https://server.duinocoin.com/users/" + String(DUCO_USER));
    Serial.println("[WALLET] Free Heap vorher: " + String(ESP.getFreeHeap()));
  #endif

#if defined(ESP8266)
  BearSSL::WiFiClientSecure client;
#else
  WiFiClientSecure client;
#endif
  client.setInsecure();
  client.setTimeout(8000);

  HTTPClient https;
  const String url = "https://server.duinocoin.com/users/" + String(DUCO_USER);

  if (!https.begin(client, url)) {
    #if defined(SERIAL_PRINTING)
      Serial.println("[WALLET] https.begin() fehlgeschlagen");
    #endif
    return false;
  }

  https.setTimeout(8000);
  https.addHeader("Accept", "application/json");

  const int httpCode = https.GET();

  #if defined(SERIAL_PRINTING)
    Serial.println("[WALLET] HTTP Code: " + String(httpCode));
  #endif

  if (httpCode != HTTP_CODE_OK) {
    #if defined(SERIAL_PRINTING)
      Serial.println("[WALLET] HTTP Fehler: " + https.errorToString(httpCode));
    #endif
    https.end();
    return false;
  }

  // Erst die komplette Antwort holen. Das ist fuer den Test absichtlich
  // einfacher und besser zu diagnostizieren als direkt aus dem TLS-Stream.
  String payload = https.getString();
  https.end();

  #if defined(SERIAL_PRINTING)
    Serial.println("[WALLET] Antwortlaenge: " + String(payload.length()));
    Serial.println("[WALLET] Free Heap nach Download: " + String(ESP.getFreeHeap()));
    Serial.println("[WALLET] JSON Anfang:");
    Serial.println(payload.substring(0, 600));
  #endif

  if (payload.length() == 0) {
    #if defined(SERIAL_PRINTING)
      Serial.println("[WALLET] Leere Serverantwort");
    #endif
    return false;
  }

  // Die aktuelle /users/<name>-Antwort enthaelt auch Transaktionen.
  // 4096 Byte sind fuer den Test bewusst grosszuegig dimensioniert.
  DynamicJsonDocument doc(4096);
  const DeserializationError err = deserializeJson(doc, payload);

  if (err) {
    #if defined(SERIAL_PRINTING)
      Serial.print("[WALLET] JSON Fehler: ");
      Serial.println(err.c_str());
    #endif
    return false;
  }

  if (!(doc["success"] | false)) {
    #if defined(SERIAL_PRINTING)
      Serial.println("[WALLET] API meldet success=false");
    #endif
    return false;
  }

  JsonObject account = doc["result"]["balance"];
  if (account.isNull()) {
    #if defined(SERIAL_PRINTING)
      Serial.println("[WALLET] result.balance fehlt");
    #endif
    return false;
  }

  JsonVariant balanceValue = account["balance"];
  if (balanceValue.isNull()) {
    #if defined(SERIAL_PRINTING)
      Serial.println("[WALLET] result.balance.balance fehlt");
    #endif
    return false;
  }

  duco_balance = balanceValue.as<float>();
  duco_max_miners = account["max_miners"] | 0;
  duco_warnings = account["warnings"] | 0;

  String verified = account["verified"] | "no";
  verified.toLowerCase();
  duco_verified = (verified == "yes" || verified == "true" || verified == "1");

  duco_wallet_valid = true;
  duco_wallet_last_update = millis();

  #if defined(SERIAL_PRINTING)
    Serial.println("[WALLET] Guthaben: " + String(duco_balance, 6) + " DUCO");
    Serial.println("[WALLET] Max Miner: " + String(duco_max_miners));
    Serial.println("[WALLET] Verified: " + String(duco_verified ? "JA" : "NEIN"));
    Serial.println("[WALLET] Warnungen: " + String(duco_warnings));
    Serial.println("[WALLET] Free Heap fertig: " + String(ESP.getFreeHeap()));
    Serial.println("[WALLET] Fertig");
    Serial.println();
  #endif

  return true;
}

// NTP-Uhr. Die Anzeige wird unabhängig vom 3-Sekunden-Miner-Refresh
// aktualisiert, damit die Uhr sekundengenau läuft.
static bool nm_clock_started = false;

static void nm_lock_display() {
#if !defined(ESP8266)
  if (nm_display_mutex != nullptr) {
    xSemaphoreTake(nm_display_mutex, portMAX_DELAY);
  }
#endif
}

static void nm_unlock_display() {
#if !defined(ESP8266)
  if (nm_display_mutex != nullptr) {
    xSemaphoreGive(nm_display_mutex);
  }
#endif
}

static void nm_start_clock() {
  if (nm_clock_started || WiFi.status() != WL_CONNECTED) return;

  // Deutsche Zeitzone inkl. Sommer-/Winterzeit.
#if defined(ESP8266)
  configTime("CET-1CEST,M3.5.0/2,M10.5.0/3",
             "pool.ntp.org", "time.nist.gov");
#else
  configTzTime("CET-1CEST,M3.5.0/2,M10.5.0/3",
               "pool.ntp.org", "time.nist.gov");
#endif
  nm_clock_started = true;
}

static String nm_clock_text() {
  struct tm timeinfo;
#if defined(ESP8266)
  const time_t now = time(nullptr);
  if (now < 100000) return "--:--";
  localtime_r(&now, &timeinfo);
#else
  if (!getLocalTime(&timeinfo, 10)) return "--:--";
#endif

  char buf[6];
  strftime(buf, sizeof(buf), "%H:%M", &timeinfo);
  return String(buf);
}

static void nm_draw_static_dashboard() {
  // Einmal komplett neu aufbauen. Danach werden nur noch die
  // tatsächlich veränderlichen Flächen gelöscht und neu gezeichnet.
  tft.fillScreen(C_BG);

  // Kopfzeile
  tft.fillRect(0, 0, 240, 27, C_PANEL2);
  nm_gfx_text("DUINO-COIN", 7, 23, &AGENCYB15pt7b, C_ACCENT);

  // Guthabenfeld. DUCO bleibt an deiner manuell eingestellten Position.
  tft.fillRoundRect(5, 30, 230, 39, 8, C_PANEL);
  nm_gfx_text("DUCO", 186, 61, &AGENCYB15pt7b, C_TEXT);

  // Reihe 1
  const int16_t leftX  = 5;
  const int16_t rightX = 123;
  const int16_t cardW  = 112;

  tft.fillRoundRect(leftX, 74, cardW, 51, 7, C_PANEL);
  nm_text("HASHRATE", 12, 79, 1, C_MUTED, C_PANEL);
  nm_text_right("kH/s", 109, 111, 1, C_GREEN, C_PANEL);

  tft.fillRoundRect(rightX, 74, cardW, 51, 7, C_PANEL);
  nm_text("DIFFICULTY", 130, 79, 1, C_MUTED, C_PANEL);

  // Reihe 2: Shares breit, Ping schmal.
  const int16_t sharesX = 5;
  const int16_t sharesW = 151;
  const int16_t pingX   = 161;
  const int16_t pingW   = 74;

  tft.fillRoundRect(sharesX, 130, sharesW, 51, 7, C_PANEL);
  nm_text("SHARES", 12, 135, 1, C_MUTED, C_PANEL);

  tft.fillRoundRect(pingX, 130, pingW, 51, 7, C_PANEL);
  nm_text("JOBS", pingX + 7, 135, 1, C_MUTED, C_PANEL);
  

  // Uhr-Blase
  tft.drawFastHLine(7, 187, 226, C_LINE);
  tft.fillRoundRect(30, 191, 180, 45, 14, C_PANEL2);
  tft.drawRoundRect(30, 191, 180, 45, 14, C_LINE);

  nm_draw_wifi_bars(199, 5);
  nm_dashboard_drawn = true;
}

// Nur die Uhr-Fläche wird gelöscht und neu geschrieben.
// Die Funktion wird oft aufgerufen, zeichnet aber nur beim Minutenwechsel neu.
// Dadurch springt z.B. 20:57 -> 20:58 direkt beim Wechsel auf Sekunde 00.
void update_display_clock() {
  static unsigned long lastClockPoll = 0;
  static String lastClockText = "";

  if (!nm_dashboard_drawn) return;

  const unsigned long now = millis();

  // Häufig prüfen, aber nur beim Minutenwechsel neu zeichnen.
  if (now - lastClockPoll < 250UL) return;
  lastClockPoll = now;

  nm_start_clock();
  const String current = nm_clock_text();

  if (current == lastClockText) return;
  lastClockText = current;

  nm_lock_display();

  // Ganze Uhr-Blase neu malen: Hintergrund + Rahmen + Text.
  tft.fillRoundRect(30, 191, 180, 45, 14, C_PANEL2);
  tft.drawRoundRect(30, 191, 180, 45, 14, C_LINE);
  nm_gfx_center(current, 120, 226, &AGENCYB20pt7b, C_TEXT);

  nm_unlock_display();
}

void display_mining_results(String hashrate_s,
                            String rejected_shares,
                            String accepted_shares,
                            String uptime_unused,
                            String node_unused,
                            String difficulty_s,
                            String sharerate,
                            String jobs_s,
                            String accept_rate) {
  nm_display_power_on();
  nm_start_clock();

  (void)uptime_unused;
  (void)node_unused;
  (void)sharerate;
  (void)accept_rate;

  static String lastBalance = "";
  static String lastHashrate = "";
  static String lastDifficulty = "";
  static String lastShares = "";
  static String lastJobs = "";
  static int lastWifiBars = -99;

  nm_lock_display();

  if (!nm_dashboard_drawn) {
    nm_draw_static_dashboard();
    lastBalance = "";
    lastHashrate = "";
    lastDifficulty = "";
    lastShares = "";
    lastJobs = "";
    lastWifiBars = -99;
  }

  // WLAN: komplette Kopfzeile neu malen, wenn sich die Balkenstufe ändert.
  int wifiBars = -1;
  if (WiFi.status() == WL_CONNECTED) {
    const int rssi = WiFi.RSSI();
    if (rssi >= -55)      wifiBars = 4;
    else if (rssi >= -67) wifiBars = 3;
    else if (rssi >= -75) wifiBars = 2;
    else if (rssi >= -85) wifiBars = 1;
    else                  wifiBars = 0;
  }

  if (wifiBars != lastWifiBars) {
    tft.fillRect(0, 0, 240, 27, C_PANEL2);
    nm_gfx_text("DUINO-COIN", 7, 23, &AGENCYB15pt7b, C_ACCENT);
    nm_draw_wifi_bars(199, 5);
    lastWifiBars = wifiBars;
  }

  // Guthaben: komplette Blase neu malen. Damit bleiben garantiert
  // keine transparenten GFX-Font-Pixel vom alten Wert stehen.
  int balanceDecimals = 6;
  if (duco_balance >= 100000.0f) {
    balanceDecimals = 3;
  } else if (duco_balance >= 10000.0f) {
    balanceDecimals = 4;
  } else if (duco_balance >= 1000.0f) {
    balanceDecimals = 5;
  }

  String balanceText = duco_wallet_valid
      ? String(duco_balance, balanceDecimals)
      : String("---.------");

  if (balanceText != lastBalance) {
    tft.fillRoundRect(5, 30, 230, 39, 8, C_PANEL);

    const GFXfont *balanceFont = &AGENCYB20pt7b;
    if (nm_gfx_width(balanceText, balanceFont) > 174) {
      balanceFont = &AGENCYB15pt7b;
    }

    nm_gfx_text(balanceText, 9, 61, balanceFont,
                duco_wallet_valid ? C_ACCENT : C_MUTED);

    // Deine manuelle DUCO-Position bleibt exakt erhalten.
    nm_gfx_text("DUCO", 186, 61, &AGENCYB15pt7b, C_TEXT);
    lastBalance = balanceText;
  }

  // Hashrate: komplette Karte neu malen.
  if (hashrate_s != lastHashrate) {
    tft.fillRoundRect(5, 74, 112, 51, 7, C_PANEL);
    nm_text("HASHRATE", 12, 79, 1, C_MUTED, C_PANEL);
    nm_gfx_text(hashrate_s, 12, 118, &AGENCYB20pt7b, C_GREEN);
    nm_text_right("kH/s", 109, 111, 1, C_GREEN, C_PANEL);
    lastHashrate = hashrate_s;
  }

  // Difficulty: komplette Karte neu malen.
  if (difficulty_s != lastDifficulty) {
    tft.fillRoundRect(123, 74, 112, 51, 7, C_PANEL);
    nm_text("DIFFICULTY", 130, 79, 1, C_MUTED, C_PANEL);
    nm_gfx_text(difficulty_s, 130, 118, &AGENCYB20pt7b, C_BLUE);
    lastDifficulty = difficulty_s;
  }

  // Shares: komplette breite Karte neu malen.
  String sharesText = rejected_shares + "/" + accepted_shares;
  if (sharesText != lastShares) {
    tft.fillRoundRect(5, 130, 151, 51, 7, C_PANEL);
    nm_text("SHARES", 12, 135, 1, C_MUTED, C_PANEL);

    const GFXfont *shareFont = nm_share_font_for(sharesText);
    const int16_t shareBaseline = (shareFont == &AGENCYB20pt7b) ? 174 : 171;
    nm_gfx_text(sharesText, 12, shareBaseline, shareFont, C_TEXT);

    lastShares = sharesText;
  }

  // JOBS: komplette schmale Karte neu malen.
  if (jobs_s != lastJobs) {
    tft.fillRoundRect(161, 130, 74, 51, 7, C_PANEL);
    nm_text("JOBS", 168, 135, 1, C_MUTED, C_PANEL);
    const GFXfont *jobsFont = (jobs_s.length() <= 4) ? &AGENCYB20pt7b : &AGENCYB15pt7b;
    nm_gfx_text(jobs_s, 168, 174, jobsFont, C_TEXT);
    lastJobs = jobs_s;
  }

  nm_unlock_display();

  // Uhr separat; sie zeichnet nur beim Minutenwechsel die komplette Blase neu.
  update_display_clock();
}

#endif // DISPLAY_NMTV154
#endif // NM_DISPLAY_H
