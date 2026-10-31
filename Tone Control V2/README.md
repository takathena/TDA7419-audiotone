# TDA7419 Tone Control via WiFi (ESP32)

Firmware ini mengubah board "KHOBE CREATIVE – Digital AudioTone Control WiFi"
(ESP32 + TDA7419) jadi tone control yang dikendalikan dari browser HP,
karena board tidak punya potensio/rotary fisik.

## Yang sudah dicek dari foto board kamu
- Chip: **TDA7419** (ST, 28-pin SOIC), I2C address tetap **0x44**.
- Jalur I2C ke ESP32: **SDA = GPIO21, SCL = GPIO22** (pin default Wire.h di ESP32).
- Header output speaker: `SUB GND R1 GND R2 GND L1 GND L2 GND`
  → diasumsikan L1=Front Left, R1=Front Right, L2=Rear Left, R2=Rear Right.
  **Cek ulang dengan multitester / dengarkan langsung**, karena urutan ini
  disimpulkan dari foto, bukan dari skematik resmi.
- Header `GND VCC SDA SCL` dan `GND CLK DT SW +` (rotary) ada di board tapi
  **tidak dipakai** oleh firmware ini karena kamu kontrol lewat web.

Kalau ternyata board kamu beda jalur SDA/SCL-nya, tinggal ubah dua baris di
`TDA7419_WebControl.ino`:
```cpp
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
```

## Isi folder ini
| File | Fungsi |
|---|---|
| `TDA7419_WebControl.ino` | Program utama: WiFi AP, web server, logic kontrol |
| `TDA7419.h` / `TDA7419.cpp` | Driver I2C khusus TDA7419 (ditulis ulang dari datasheet resmi) |
| `webpage.h` | Halaman HTML/CSS/JS kontrolnya (sudah ditanam di firmware, tidak perlu file terpisah) |

## Cara pakai
1. Buka Arduino IDE → **File > Open** → pilih `TDA7419_WebControl.ino`
   (pastikan ketiga file lain tetap satu folder yang sama, Arduino akan
   otomatis membukanya sebagai tab terpisah).
2. Kalau belum ada, install board ESP32:
   **File > Preferences** → tambahkan URL board manager:
   `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   lalu **Tools > Board > Boards Manager** → cari "esp32" → install.
3. Pilih **Tools > Board** sesuai modul ESP32 di PCB (umumnya "ESP32 Dev Module").
4. Pilih port COM yang sesuai (sambungkan ESP32 via USB-C ke laptop).
5. Klik **Upload**.
6. Buka **Serial Monitor** (115200 baud) — akan muncul info WiFi & IP.
7. Di HP: connect ke WiFi **`TDA7419-Tone-Control`**, password **`audiotone123`**
   (bisa diganti di baris `AP_SSID` / `AP_PASS` pada file .ino).
8. Buka browser, ketik `http://192.168.4.1` → muncul halaman kontrol.

## Kontrol yang tersedia di halaman web
- **Sumber Input**: pilih antara QD / SE1 / SE2 / SE3 (3 input single-ended +
  1 input differential bawaan chip). Cek fisik kabel mana yang nyambung ke
  input mana di board kamu.
- **Mute** — mematikan suara halus (soft-mute bawaan chip, tidak langsung putus).
- **Loudness** — penguatan bass/treble otomatis saat volume kecil (on/off).
- **Volume** (-79 .. +15 dB) — volume utama/master.
- **Balance** — geser ke kiri/kanan.
- **Fader** — geser ke depan/belakang (4 speaker independen).
- **Bass / Middle / Treble** (-15 .. +15 dB).
- **Subwoofer** (-79 .. +15 dB) — level output subwoofer.

Semua perubahan otomatis **disimpan ke flash ESP32** (pakai `Preferences`),
jadi settingan terakhir tetap ada walau board dimatikan/restart.

## ⚠️ Yang perlu kamu tes & verifikasi sendiri
Karena saya tidak punya board fisiknya, beberapa hal berikut **perlu diuji
langsung** dan mungkin perlu penyesuaian kecil:

1. **Polaritas tombol Mute** — kalau pas ditekan malah suara jadi hidup
   (kebalik), buka `TDA7419.cpp`, cari fungsi `setSoftMute()`, lalu tukar
   `0x00` dan `0x01` di baris tersebut. Sudah saya beri komentar di situ.
2. **Pemetaan channel speaker** (LF/RF/LR/RR ke L1/L2/R1/R2 fisik) — kalau
   geser Balance/Fader ternyata arahnya terbalik, tinggal tukar urutan di
   fungsi `applySpeakers()` pada file `.ino`.
3. **Volume awal sengaja dibuat pelan (-30dB)** supaya aman untuk speaker
   saat pertama kali tes. Naikkan pelan-pelan dari HP setelah yakin semua
   beres.
4. **Password WiFi default** `audiotone123` — ganti sebelum dipakai di luar
   rumah/sekolah.

## Pengembangan lanjutan (opsional, belum dibuatkan)
- Mode WiFi **Station** (connect ke WiFi rumah, bukan bikin AP sendiri) —
  supaya bisa dikontrol dari internet/HP yang sudah connect WiFi lain.
  Tinggal ganti `WiFi.softAP()` jadi `WiFi.begin(ssid, pass)`.
- Tampilan di LCD I2C (header `LCD` di board sudah ada, satu bus I2C
  dengan TDA7419, alamat I2C LCD biasanya `0x27` atau `0x3F`, tidak akan
  bentrok dengan TDA7419 yang alamatnya `0x44`).
- Equalizer 7-band pakai fitur **Spectrum Analyzer** bawaan chip (register
  sudah disiapkan di datasheet, belum diimplementasi di driver ini).

## 🔵 Bluetooth (A2DP) sebagai sumber audio — BARU

Firmware sekarang bisa menerima audio dari HP lewat Bluetooth (protokol A2DP,
sama seperti speaker Bluetooth biasa), lalu disalurkan ke TDA7419 sebagai
salah satu sumber input. **Wajib baca dulu**, karena ini butuh 1 keputusan
hardware dan 1 langkah instalasi library manual — detail lengkap ada di
komentar atas file `TDA7419_Bluetooth.h`, ringkasannya:

### Kenapa tidak bisa langsung "plug & play"
TDA7419 cuma punya input **analog** (SE1/SE2/SE3/QD), sedangkan audio
Bluetooth datang dalam bentuk digital. Jadi harus diubah ke analog dulu
oleh ESP32, baru disambung ke salah satu input itu.

### Pilih salah satu (atur lewat `BT_USE_INTERNAL_DAC` di `TDA7419_Bluetooth.h`)
| | Opsi A — DAC internal ESP32 (default) | Opsi B — Modul DAC I2S eksternal |
|---|---|---|
| Komponen tambahan | Tidak ada (pakai GPIO25/26 yang sudah ada di header) | Modul PCM5102A/UDA1334A (~15-40rb) |
| Kualitas | 8-bit, agak noise tapi cukup jelas | Jauh lebih bersih (16-bit) |
| Wiring | GPIO25→L3, GPIO26→R3 (lewat kapasitor kopling) | BCK/WS/DOUT modul → GPIO14/15/22, lalu output analog modul → L3/R3 |

Kedua opsi tetap butuh **kapasitor kopling** (1–2.2µF) di jalur sinyal
sebelum masuk ke pin input TDA7419, untuk memblokir DC dan mengurangi noise.

### Channel fisik yang "dikorbankan"
Defaultnya kode memakai jalur **SE3** (`BT_INPUT_CHANNEL`) untuk Bluetooth.
⚠️ **Cek dulu board kamu** — kalau SE3 sudah terpakai kabel RCA/aux lain,
ganti `BT_INPUT_CHANNEL` ke `TDA_IN_QD` atau channel lain yang masih kosong.

### Library tambahan (wajib install manual, beda dari file lain di project ini)
Arduino IDE → **Tools → Manage Libraries** → cari & install:
1. `ESP32-A2DP` (by pschatzmann)
2. `AudioTools` (by pschatzmann) — dependency wajib untuk library di atas

### Cara pakai di HP
1. Pairing Bluetooth HP ke perangkat bernama **"TDA7419-Tone-Control"**
   (nama sama persis dengan SSID WiFi-nya, tapi ini koneksi Bluetooth
   terpisah — kamu tetap konek WiFi yang sama untuk buka halaman kontrol).
2. Putar musik seperti biasa dari HP.
3. Di halaman web, pilih **"Bluetooth (A2DP)"** di dropdown Sumber Input.
4. Kartu "Bluetooth" di halaman web menampilkan status terhubung +
   judul/artis lagu yang sedang main (kalau HP mengirim metadata-nya).

### Yang perlu kamu tes & verifikasi sendiri (tambahan)
5. **Koeksistensi WiFi + Bluetooth** — keduanya jalan di radio 2.4GHz yang
   sama. Biasanya aman, tapi kalau jangkauan WiFi terasa menurun saat
   Bluetooth streaming aktif, itu wajar (trade-off bawaan chip ESP32).
6. Kalau suara dari Bluetooth pelan/berisik di opsi DAC internal, itu
   batas kualitas 8-bit DAC-nya — upgrade ke opsi B (DAC I2S eksternal)
   kalau butuh kualitas lebih baik.
