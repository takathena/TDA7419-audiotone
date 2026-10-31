/*
  TDA7419_WebControl.ino
  ------------------------------------------------------------
  Firmware ESP32 untuk mengendalikan chip TDA7419 (3-band car
  audio tone processor) lewat halaman web, karena board tidak
  dilengkapi potensiometer/rotary fisik.

  ESP32 akan membuat WiFi Access Point sendiri. HP tinggal connect
  ke WiFi tsb lalu buka browser ke alamat yang ditampilkan di
  Serial Monitor (default 192.168.4.1) -- halaman kontrol akan
  muncul, tidak perlu install aplikasi apapun.

  Wiring (berdasarkan jalur PCB "KHOBE CREATIVE AudioTone WiFi"):
    TDA7419 SDA  -> ESP32 GPIO21
    TDA7419 SCL  -> ESP32 GPIO22
    TDA7419 I2C address tetap: 0x44

  Library yang dibutuhkan (semua sudah bawaan ESP32 core,
  tidak perlu install tambahan apapun di Library Manager):
    - WiFi.h
    - WebServer.h
    - Wire.h
    - Preferences.h

  Cara pakai:
    1. Install "ESP32 board support" di Arduino IDE (Boards Manager)
       kalau belum ada.
    2. Pilih board sesuai modul ESP32 di PCB (biasanya "ESP32 Dev Module").
    3. Upload sketch ini (folder ini berisi 4 file: .ino, TDA7419.h,
       TDA7419.cpp, webpage.h -- biarkan dalam satu folder yang sama).
    4. Buka Serial Monitor (115200 baud) untuk lihat status & IP.
    5. Di HP, connect ke WiFi "TDA7419-Tone-Control" (password di bawah).
    6. Buka browser ke http://192.168.4.1
*/

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include "TDA7419.h"
#include "webpage.h"

// ---------- KONFIGURASI ----------
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22

const char* AP_SSID = "TDA7419-Tone-Control";
const char* AP_PASS = "audiotone123";   // minimal 8 karakter

// ---------- OBJEK GLOBAL ----------
TDA7419 tda;
WebServer server(80);
Preferences prefs;

// ---------- STATE (nilai yang sedang aktif) ----------
struct ToneState {
  int volume  = -30;  // -79..15 dB
  int bass    = 0;    // -15..15 dB
  int middle  = 0;    // -15..15 dB
  int treble  = 0;    // -15..15 dB
  int balance = 0;    // -40..40 (negatif = ke kiri)
  int fader   = 0;    // -40..40 (negatif = ke belakang)
  int sub     = 0;    // -79..15 dB
  int input   = 1;    // 0=QD,1=SE1,2=SE2,3=SE3
  bool mute   = false;
  bool loud   = false;
} state;

// ---------- Hitung & kirim ulang semua channel speaker dari volume+balance+fader ----------
void applySpeakers() {
  // Balance/fader bekerja dengan meredam sisi yang "dijauhi", bukan menambah gain,
  // supaya tidak pernah melebihi batas +15dB chip secara tidak sengaja.
  int attL = max(0, state.balance);   // balance positif -> kanan lebih keras -> kiri diredam
  int attR = max(0, -state.balance);
  int attF = max(0, state.fader);     // fader positif -> belakang lebih keras -> depan diredam
  int attB = max(0, -state.fader);

  int lf = -attL - attF;
  int rf = -attR - attF;
  int lr = -attL - attB;
  int rr = -attR - attB;

  tda.setSpeaker(TDA_CH_LF, lf);
  tda.setSpeaker(TDA_CH_RF, rf);
  tda.setSpeaker(TDA_CH_LR, lr);
  tda.setSpeaker(TDA_CH_RR, rr);
}

void applyAll() {
  tda.setInput((uint8_t)state.input, 0, true);
  tda.setVolume(state.volume);
  tda.setBass(state.bass);
  tda.setMiddle(state.middle);
  tda.setTreble(state.treble);
  tda.setSubwoofer(state.sub);
  tda.setLoudness(state.loud ? 10 : 0, 1, state.loud, true);
  applySpeakers();
  tda.setSoftMute(state.mute);
}

void saveState() {
  prefs.putInt("volume", state.volume);
  prefs.putInt("bass", state.bass);
  prefs.putInt("middle", state.middle);
  prefs.putInt("treble", state.treble);
  prefs.putInt("balance", state.balance);
  prefs.putInt("fader", state.fader);
  prefs.putInt("sub", state.sub);
  prefs.putInt("input", state.input);
  prefs.putBool("mute", state.mute);
  prefs.putBool("loud", state.loud);
}

void loadState() {
  state.volume  = prefs.getInt("volume", -30);
  state.bass    = prefs.getInt("bass", 0);
  state.middle  = prefs.getInt("middle", 0);
  state.treble  = prefs.getInt("treble", 0);
  state.balance = prefs.getInt("balance", 0);
  state.fader   = prefs.getInt("fader", 0);
  state.sub     = prefs.getInt("sub", 0);
  state.input   = prefs.getInt("input", 1);
  state.mute    = prefs.getBool("mute", false);
  state.loud    = prefs.getBool("loud", false);
}

// ---------- HANDLER WEB ----------
void handleRoot() {
  server.send_P(200, "text/html", MAIN_PAGE);
}

void handleState() {
  String json = "{";
  json += "\"volume\":" + String(state.volume) + ",";
  json += "\"bass\":" + String(state.bass) + ",";
  json += "\"middle\":" + String(state.middle) + ",";
  json += "\"treble\":" + String(state.treble) + ",";
  json += "\"balance\":" + String(state.balance) + ",";
  json += "\"fader\":" + String(state.fader) + ",";
  json += "\"sub\":" + String(state.sub) + ",";
  json += "\"input\":" + String(state.input) + ",";
  json += "\"mute\":" + String(state.mute ? "true" : "false") + ",";
  json += "\"loud\":" + String(state.loud ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

void handleSet() {
  bool touched = false;

  if (server.hasArg("volume"))  { state.volume  = constrain(server.arg("volume").toInt(), -79, 15); tda.setVolume(state.volume); touched = true; }
  if (server.hasArg("bass"))    { state.bass    = constrain(server.arg("bass").toInt(), -15, 15);    tda.setBass(state.bass); touched = true; }
  if (server.hasArg("middle"))  { state.middle  = constrain(server.arg("middle").toInt(), -15, 15);  tda.setMiddle(state.middle); touched = true; }
  if (server.hasArg("treble"))  { state.treble  = constrain(server.arg("treble").toInt(), -15, 15);  tda.setTreble(state.treble); touched = true; }
  if (server.hasArg("sub"))     { state.sub     = constrain(server.arg("sub").toInt(), -79, 15);     tda.setSubwoofer(state.sub); touched = true; }
  if (server.hasArg("input"))   { state.input   = constrain(server.arg("input").toInt(), 0, 3);      tda.setInput((uint8_t)state.input); touched = true; }
  if (server.hasArg("mute"))    { state.mute    = server.arg("mute").toInt() != 0;                   tda.setSoftMute(state.mute); touched = true; }
  if (server.hasArg("loud"))    { state.loud    = server.arg("loud").toInt() != 0;
                                   tda.setLoudness(state.loud ? 10 : 0, 1, state.loud, true); touched = true; }
  if (server.hasArg("balance")) { state.balance = constrain(server.arg("balance").toInt(), -40, 40); applySpeakers(); touched = true; }
  if (server.hasArg("fader"))   { state.fader   = constrain(server.arg("fader").toInt(), -40, 40);   applySpeakers(); touched = true; }

  if (touched) saveState();
  server.send(200, "text/plain", "OK");
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

// ---------- SETUP & LOOP ----------
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n[TDA7419] Booting...");

  prefs.begin("tda7419", false);
  loadState();

  tda.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  tda.setDefaults();   // set register awal (aman/senyap dulu)
  applyAll();           // lalu timpa dengan nilai tersimpan terakhir

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  IPAddress ip = WiFi.softAPIP();
  Serial.print("[WiFi] Access Point aktif: "); Serial.println(AP_SSID);
  Serial.print("[WiFi] Password: "); Serial.println(AP_PASS);
  Serial.print("[WiFi] Buka browser ke: http://"); Serial.println(ip);

  server.on("/", handleRoot);
  server.on("/api/state", handleState);
  server.on("/api/set", handleSet);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("[HTTP] Server siap.");
}

void loop() {
  server.handleClient();
}
