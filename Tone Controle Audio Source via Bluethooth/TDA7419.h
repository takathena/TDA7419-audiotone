// TDA7419.h
// Driver I2C bersih untuk ST TDA7419 (3-band car audio processor)
// Ditulis ulang berdasarkan datasheet resmi (STMicroelectronics, Sep 2013, Rev 7)
// Dasar referensi awal: proyek TDA7419 oleh Aleksandr Liman (liman324@yandex.ru)
//
// Semua fungsi "dB" di sini pakai satuan desibel asli (bukan kode register mentah),
// supaya aman dipakai dari kode UI web tanpa perlu mikir bit-bit register.

#ifndef TDA7419_H
#define TDA7419_H

#include <Arduino.h>

// Alamat I2C 7-bit tetap (lihat datasheet bagian 5.1)
#define TDA7419_I2C_ADDR 0x44

// Sub-address register (datasheet Table 7)
#define TDA7419_REG_INPUT1      0x40 // Main source selector
#define TDA7419_REG_LOUDNESS    0x41 // Main loudness
#define TDA7419_REG_SOFT        0x42 // Soft-mute / clock generator
#define TDA7419_REG_VOLUME      0x43 // Volume
#define TDA7419_REG_TREBLE      0x44 // Treble
#define TDA7419_REG_MIDDLE      0x45 // Middle
#define TDA7419_REG_BASS        0x46 // Bass
#define TDA7419_REG_INPUT2      0x47 // Second source selector
#define TDA7419_REG_SUB_MID_BAS 0x48 // Subwoofer / middle / bass center freq
#define TDA7419_REG_MIX_GAIN    0x49 // Mixing / gain effect
#define TDA7419_REG_ATT_LF      0x4A // Speaker attenuator Left Front
#define TDA7419_REG_ATT_RF      0x4B // Speaker attenuator Right Front
#define TDA7419_REG_ATT_LR      0x4C // Speaker attenuator Left Rear
#define TDA7419_REG_ATT_RR      0x4D // Speaker attenuator Right Rear
#define TDA7419_REG_MIX_LEVEL   0x4E // Mixing level control
#define TDA7419_REG_ATT_SUB     0x4F // Subwoofer attenuator
#define TDA7419_REG_SPECTRUM    0x50 // Spectrum analyzer / clock / AC mode

// Pilihan sumber input (Main/Second source selector), datasheet Table 8 & 15
enum TDA_Input {
  TDA_IN_QD   = 0, // Quasi-Differential input
  TDA_IN_SE1  = 1, // Single-Ended input 1
  TDA_IN_SE2  = 2, // Single-Ended input 2
  TDA_IN_SE3  = 3, // Single-Ended input 3 / AC2IN
  TDA_IN_SE   = 4, // QD pins dipakai sebagai Single-Ended
  TDA_IN_MUTE = 5  // Input dimute
};

// Channel speaker independen
enum TDA_Channel {
  TDA_CH_LF = 0, // Left Front
  TDA_CH_RF = 1, // Right Front
  TDA_CH_LR = 2, // Left Rear
  TDA_CH_RR = 3  // Right Rear
};

class TDA7419 {
  public:
    TDA7419();

    // Wajib dipanggil di setup(). sda/scl sesuai jalur PCB (default GPIO21/22).
    void begin(int sda = 21, int scl = 22);

    // Set semua register ke kondisi default yang aman (volume rendah, flat EQ, unmuted siap pakai)
    void setDefaults();

    // --- Input ---
    // source: TDA_IN_*, gainDb: 0..15 (penguatan input tambahan), autoZero: true=aktif
    void setInput(uint8_t source, uint8_t gainDb = 0, bool autoZero = true);
    void setInput2(uint8_t source, uint8_t gainDb = 0, bool autoZero = true);

    // --- Volume utama ---
    // dB: -79 .. +15
    void setVolume(int dB, bool softStep = true);

    // --- 4 output speaker independen ---
    // dB: -79 .. +15 (0 = flat / tidak diredam)
    void setSpeaker(uint8_t channel, int dB, bool softStep = true);

    // --- Subwoofer ---
    void setSubwoofer(int dB, bool softStep = true);
    // cutoff: 0=flat,1=80Hz,2=120Hz,3=160Hz
    void setSubwooferCutoff(uint8_t cutoffIdx);

    // --- Bass ---
    // dB: -15..+15, freqIdx: 0=60Hz,1=80Hz,2=100Hz,3=200Hz, qIdx: 0=1.0,1=1.25,2=1.5,3=2.0
    void setBass(int dB, uint8_t freqIdx = 1, uint8_t qIdx = 0, bool softStep = true);

    // --- Middle ---
    // dB: -15..+15, qIdx: 0=0.5,1=0.75,2=1.0,3=1.25
    void setMiddle(int dB, uint8_t qIdx = 2, bool softStep = true);
    void setMiddleFreq(uint8_t freqIdx); // 0=500Hz,1=1kHz,2=1.5kHz,3=2.5kHz

    // --- Treble ---
    // dB: -15..+15, freqIdx: 0=10kHz,1=12.5kHz,2=15kHz,3=17.5kHz
    void setTreble(int dB, uint8_t freqIdx = 3);

    // --- Loudness ---
    // attenDb: 0..15 (besar kecil efek loudness), freqIdx: 0=flat,1=400Hz,2=800Hz,3=2400Hz
    void setLoudness(uint8_t attenDb, uint8_t freqIdx = 1, bool highBoost = false, bool softStep = true);

    // --- Mute global via fitur Soft-Mute bawaan chip (fade halus) ---
    void setSoftMute(bool muteOn);

    // --- Mixing depan (jarang dipakai, disediakan untuk kelengkapan) ---
    void setMixLevel(int dB, bool softStep = true);
    void setMixEnable(bool leftOn, bool rightOn, bool enable);

  private:
    int _sda, _scl;
    void writeReg(uint8_t subaddr, uint8_t data);
    // Konversi dB (-79..+15) ke kode register 7-bit sesuai Table 11 datasheet
    uint8_t gainCode(int dB);
    // Konversi dB (-15..+15) ke kode register sesuai Table 12/13/14 (bit4 = sign)
    uint8_t filterCode(int dB);
};

#endif // TDA7419_H
