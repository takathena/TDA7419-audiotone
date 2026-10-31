// TDA7419_Bluetooth.h
// ------------------------------------------------------------
// Menambahkan Bluetooth (A2DP) sebagai salah satu sumber audio.
// ------------------------------------------------------------
//
// BACA DULU SEBELUM UPLOAD -- ini bukan colok-jalan, ada 1 keputusan
// hardware yang HARUS kamu ambil dulu:
//
// TDA7419 TIDAK punya input digital. Chip ini cuma mau terima sinyal
// ANALOG di pin SE1/SE2/SE3/QD. Audio Bluetooth datang dalam bentuk
// digital (lewat protokol A2DP/SBC), jadi HARUS diubah ke analog dulu
// sebelum bisa masuk ke salah satu input itu. Dua cara, pilih salah
// satu lewat BT_USE_INTERNAL_DAC di bawah:
//
//   [A] BT_USE_INTERNAL_DAC = 1  (DEFAULT)
//       Pakai DAC internal ESP32 di pin GPIO25 & GPIO26. Pin ini SUDAH
//       keluar di header board kamu (label "D25" & "D26" di baris atas
//       header ESP32) -- jadi 0 komponen tambahan, tinggal jumper.
//       Kualitas: 8-bit, ada sedikit noise/desis tapi cukup jelas untuk
//       audio mobil. Cocok untuk langsung coba dulu.
//
//   [B] BT_USE_INTERNAL_DAC = 0
//       Pakai modul DAC I2S eksternal (PCM5102A / UDA1334A / MAX98357,
//       harga ~15-40rb di marketplace). Kualitas jauh lebih baik
//       (16-bit asli, noise rendah). Butuh 3 kabel data (BCK/WS/DOUT)
//       + VCC 3.3V + GND ke modul, baru output analog modul itu yang
//       disambung ke TDA7419. Pin default di bawah pakai GPIO14 (BCK),
//       GPIO15 (WS), GPIO22 (DOUT) -- cek dulu di foto board kamu pin
//       itu belum dipakai jalur lain, lalu sesuaikan kalau perlu.
//
// SETELAH itu, keluaran analognya (baik dari opsi A atau B) WAJIB
// disambung ke salah satu pasangan pin L/R di header "INPUT AUDIO" pada
// board kamu (GND L3 GND R3 GND L2 GND R2 GND L1 GND R1 ...), lewat
// kapasitor kopling (lihat catatan wiring di bawah).
//
// Kode ini defaultnya mengasumsikan kamu mengorbankan jalur SE3 (L3/R3)
// untuk Bluetooth -- diatur lewat BT_INPUT_CHANNEL. CEK DULU FISIK BOARD
// KAMU: kalau SE3 ternyata sudah dipakai kabel RCA lain, ganti
// BT_INPUT_CHANNEL ke TDA_IN_QD atau channel lain yang masih kosong.
//
// WIRING KAPASITOR KOPLING (wajib, jangan disambung langsung):
//   ESP32 GPIO25 (atau DOUT kiri modul DAC) --[C 1-2.2uF]--> pin L3 (SE3L)
//   ESP32 GPIO26 (atau DOUT kanan modul DAC) --[C 1-2.2uF]--> pin R3 (SE3R)
//   GND DAC -> GND board (salah satu pin GND di header INPUT AUDIO)
//   Kapasitor ini memblokir tegangan DC dari DAC supaya tidak membebani
//   input TDA7419 dan menghilangkan noise akibat offset DC.
//
// LIBRARY YANG HARUS DIINSTALL MANUAL (beda dari file lain di project
// ini yang semuanya built-in ESP32 core):
//   Arduino IDE > Tools > Manage Libraries > cari & install:
//     1. "ESP32-A2DP"      oleh pschatzmann
//     2. "AudioTools"      oleh pschatzmann   (dependency wajib ESP32-A2DP)
//
// KOMPATIBILITAS MODUL: dari foto board kamu, modul ESP32 yang dipakai
// punya pin VP/VN/D33/D35 (ciri khas ESP32 WROOM/WROVER "classic") --
// ini COCOK, karena fitur A2DP cuma jalan di ESP32 classic. Fitur ini
// TIDAK akan jalan di modul ESP32-S2/S3/C3 (chip itu cuma punya BLE,
// tidak punya Bluetooth Classic yang dibutuhkan A2DP).
//
// CATATAN: WiFi Access Point & Bluetooth jalan bersamaan di radio 2.4GHz
// yang sama (ESP32 mendukung ini / "coexistence"), tapi kadang jangkauan
// atau kestabilan WiFi sedikit menurun saat Bluetooth lagi streaming
// aktif. Ini wajar, bukan bug.

#ifndef TDA7419_BLUETOOTH_H
#define TDA7419_BLUETOOTH_H

#include "AudioTools.h"
#include "BluetoothA2DPSink.h"
#include "TDA7419.h"

// ---------- PILIHAN HARDWARE DAC ----------
#define BT_USE_INTERNAL_DAC 1     // 1 = DAC internal ESP32 (opsi A), 0 = DAC I2S eksternal (opsi B)

// Dipakai hanya kalau BT_USE_INTERNAL_DAC = 0:
#define BT_I2S_BCK_PIN   14
#define BT_I2S_WS_PIN    15
#define BT_I2S_DATA_PIN  22

// Channel fisik TDA7419 yang dikorbankan untuk Bluetooth (default SE3).
// Ganti ke TDA_IN_QD / TDA_IN_SE1 / TDA_IN_SE2 kalau SE3 sudah terpakai.
#define BT_INPUT_CHANNEL TDA_IN_SE3

// Nama yang muncul di daftar Bluetooth HP saat pairing
#define BT_DEVICE_NAME "TDA7419-Tone-Control"

// ---------- OBJEK AUDIO ----------
#if BT_USE_INTERNAL_DAC
  AnalogAudioStream btOut;
#else
  I2SStream btOut;
#endif
BluetoothA2DPSink a2dp_sink(btOut);

// ---------- STATUS (dibaca & ditampilkan di web UI) ----------
volatile bool btConnected = false;
String btTrackTitle  = "";
String btTrackArtist = "";

void btConnectionStateChanged(esp_a2d_connection_state_t state, void *ptr) {
  btConnected = (state == ESP_A2D_CONNECTION_STATE_CONNECTED);
  if (!btConnected) { btTrackTitle = ""; btTrackArtist = ""; }
  Serial.print("[BT] Status koneksi: ");
  Serial.println(btConnected ? "Terhubung" : "Terputus");
}

void btMetadataChanged(uint8_t id, const uint8_t *text) {
  String val = String((const char*)text);
  if (id == ESP_AVRC_MD_ATTR_TITLE)  btTrackTitle  = val;
  if (id == ESP_AVRC_MD_ATTR_ARTIST) btTrackArtist = val;
}

void startBluetoothA2DP() {
#if BT_USE_INTERNAL_DAC
  auto cfg = btOut.defaultConfig();
  btOut.begin(cfg);   // output analog otomatis di GPIO25 (kiri) & GPIO26 (kanan)
#else
  auto cfg = btOut.defaultConfig();
  cfg.pin_bck  = BT_I2S_BCK_PIN;
  cfg.pin_ws   = BT_I2S_WS_PIN;
  cfg.pin_data = BT_I2S_DATA_PIN;
  btOut.begin(cfg);
#endif

  a2dp_sink.set_on_connection_state_changed(btConnectionStateChanged);
  a2dp_sink.set_avrc_metadata_callback(btMetadataChanged);
  a2dp_sink.set_auto_reconnect(true);
  a2dp_sink.start(BT_DEVICE_NAME);

  Serial.print("[BT] A2DP sink aktif sebagai \"");
  Serial.print(BT_DEVICE_NAME);
  Serial.println("\", menunggu koneksi dari HP...");
}

#endif // TDA7419_BLUETOOTH_H
