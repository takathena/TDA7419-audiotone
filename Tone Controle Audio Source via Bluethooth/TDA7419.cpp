// TDA7419.cpp
#include <Wire.h>
#include "TDA7419.h"

TDA7419::TDA7419() {}

void TDA7419::begin(int sda, int scl) {
  _sda = sda;
  _scl = scl;
  Wire.begin(_sda, _scl);
  Wire.setClock(100000); // 100kHz aman, datasheet maks 500kbit/s
}

void TDA7419::writeReg(uint8_t subaddr, uint8_t data) {
  Wire.beginTransmission(TDA7419_I2C_ADDR);
  Wire.write(subaddr);
  Wire.write(data);
  Wire.endTransmission();
}

// Datasheet Table 11: 0..15 -> +0..+15dB ; 16..95 -> -0..-79dB (bit7 dipakai terpisah utk soft-step)
uint8_t TDA7419::gainCode(int dB) {
  if (dB > 15) dB = 15;
  if (dB < -79) dB = -79;
  if (dB >= 0) return (uint8_t)dB;
  return (uint8_t)(16 + (-dB));
}

// Datasheet Table 12/13/14: 0..15 -> -15..-1..0dB(bit4=1,D3..D0=15..0) ; 0x10..0x1F -> 0..+15dB(bit4=1)
// Implementasi sesuai kode asli pabrikan: nilai 1..15 negatif = D4=0,D3..D0=magnitude ; 0 dan positif = D4=1,D3..D0=nilai
uint8_t TDA7419::filterCode(int dB) {
  if (dB > 15) dB = 15;
  if (dB < -15) dB = -15;
  if (dB < 0) return (uint8_t)(-dB) & 0x0F;          // D4=0, magnitude di D3..D0
  return (uint8_t)(0x10 | (dB & 0x0F));               // D4=1, nilai di D3..D0
}

void TDA7419::setDefaults() {
  setInput(TDA_IN_SE1, 0, true);
  setInput2(TDA_IN_SE1, 0, true);
  // Soft-mute: waktu mute 123ms, soft-step blend 1.28ms, kontrol lewat I2C saja
  writeReg(TDA7419_REG_SOFT, 0b00111010);
  setVolume(-30, true);           // mulai pelan demi keamanan speaker
  setBass(0, 1, 0, true);
  setMiddle(0, 2, true);
  setMiddleFreq(1);
  setTreble(0, 3);
  setLoudness(0, 1, false, true);
  setSpeaker(TDA_CH_LF, 0, true);
  setSpeaker(TDA_CH_RF, 0, true);
  setSpeaker(TDA_CH_LR, 0, true);
  setSpeaker(TDA_CH_RR, 0, true);
  setSubwoofer(0, true);
  setSubwooferCutoff(1);
  setMixEnable(false, false, false);
}

void TDA7419::setInput(uint8_t source, uint8_t gainDb, bool autoZero) {
  if (gainDb > 15) gainDb = 15;
  uint8_t b = (uint8_t)((autoZero ? 0 : 1) << 7) | ((gainDb & 0x0F) << 3) | (source & 0x07);
  writeReg(TDA7419_REG_INPUT1, b);
}

void TDA7419::setInput2(uint8_t source, uint8_t gainDb, bool autoZero) {
  if (gainDb > 15) gainDb = 15;
  uint8_t b = (uint8_t)((autoZero ? 0 : 1) << 7) | ((gainDb & 0x0F) << 3) | (source & 0x07);
  writeReg(TDA7419_REG_INPUT2, b);
}

void TDA7419::setVolume(int dB, bool softStep) {
  uint8_t code = gainCode(dB) | (softStep ? 0x00 : 0x80);
  writeReg(TDA7419_REG_VOLUME, code);
}

void TDA7419::setSpeaker(uint8_t channel, int dB, bool softStep) {
  uint8_t code = gainCode(dB) | (softStep ? 0x00 : 0x80);
  uint8_t reg;
  switch (channel) {
    case TDA_CH_LF: reg = TDA7419_REG_ATT_LF; break;
    case TDA_CH_RF: reg = TDA7419_REG_ATT_RF; break;
    case TDA_CH_LR: reg = TDA7419_REG_ATT_LR; break;
    default:        reg = TDA7419_REG_ATT_RR; break;
  }
  writeReg(reg, code);
}

void TDA7419::setSubwoofer(int dB, bool softStep) {
  uint8_t code = gainCode(dB) | (softStep ? 0x00 : 0x80);
  writeReg(TDA7419_REG_ATT_SUB, code);
}

void TDA7419::setMixLevel(int dB, bool softStep) {
  uint8_t code = gainCode(dB) | (softStep ? 0x00 : 0x80);
  writeReg(TDA7419_REG_MIX_LEVEL, code);
}

// Register 0x48 (Sub/Mid/Bass) dipakai bersama oleh cutoff subwoofer, freq middle, freq bass.
// Supaya tidak saling menimpa, kita simpan state-nya di sini dan tulis ulang utuh tiap kali salah satu berubah.
static uint8_t s_subCutoffIdx = 1;
static uint8_t s_midFreqIdx   = 1;
static uint8_t s_bassFreqIdx  = 1;
static bool    s_bassDcOff    = true;   // true = DC mode off (normal)
static bool    s_smoothOn     = true;   // smoothing filter on

static uint8_t buildSubMidBassByte() {
  // Layout (datasheet Table 16): D1:D0=subwoofer cutoff, D3:D2=middle freq,
  // D5:D4=bass freq (keempatnya field 2-bit), D6=bass DC mode, D7=smoothing filter
  uint8_t b = 0;
  b |= (s_subCutoffIdx & 0x03);
  b |= (s_midFreqIdx & 0x03) << 2;
  b |= (s_bassFreqIdx & 0x03) << 4;
  if (s_bassDcOff) b |= 0x40; // bit6 = bass DC mode (1=off/normal, 0=on/shelving-cut)
  if (s_smoothOn)  b |= 0x80; // bit7 = smoothing filter (1=on, 0=off/bypass)
  return b;
}

void TDA7419::setSubwooferCutoff(uint8_t cutoffIdx) {
  s_subCutoffIdx = cutoffIdx & 0x03;
  writeReg(TDA7419_REG_SUB_MID_BAS, buildSubMidBassByte());
}

void TDA7419::setMiddleFreq(uint8_t freqIdx) {
  s_midFreqIdx = freqIdx & 0x03;
  writeReg(TDA7419_REG_SUB_MID_BAS, buildSubMidBassByte());
}

void TDA7419::setBass(int dB, uint8_t freqIdx, uint8_t qIdx, bool softStep) {
  s_bassFreqIdx = freqIdx & 0x03;
  writeReg(TDA7419_REG_SUB_MID_BAS, buildSubMidBassByte());
  uint8_t code = filterCode(dB) | ((qIdx & 0x03) << 5) | (softStep ? 0x00 : 0x80);
  writeReg(TDA7419_REG_BASS, code);
}

void TDA7419::setMiddle(int dB, uint8_t qIdx, bool softStep) {
  uint8_t code = filterCode(dB) | ((qIdx & 0x03) << 5) | (softStep ? 0x00 : 0x80);
  writeReg(TDA7419_REG_MIDDLE, code);
}

void TDA7419::setTreble(int dB, uint8_t freqIdx) {
  uint8_t code = filterCode(dB) | ((freqIdx & 0x03) << 5);
  writeReg(TDA7419_REG_TREBLE, code); // bit7 (Vref select) dibiarkan 0 = pakai Vref eksternal default
}

void TDA7419::setLoudness(uint8_t attenDb, uint8_t freqIdx, bool highBoost, bool softStep) {
  if (attenDb > 15) attenDb = 15;
  uint8_t b = (attenDb & 0x0F)
            | ((freqIdx & 0x03) << 4)
            | (highBoost ? 0x40 : 0x00)
            | (softStep ? 0x00 : 0x80);
  writeReg(TDA7419_REG_LOUDNESS, b);
}

void TDA7419::setSoftMute(bool muteOn) {
  // CATATAN: polaritas bit0 (soft-mute) di datasheet tertulis "0=on / 1=off" tapi tidak
  // dijelaskan tegas apakah "on" berarti "fitur aktif" atau "sedang dalam kondisi mute".
  // Kode di bawah mengasumsikan bit0=0 -> output DIMUTE. Jika saat dites ternyata
  // kebalik (tombol Mute malah menghidupkan suara), tinggal tukar 0x00 <-> 0x01 di sini.
  uint8_t b = (muteOn ? 0x00 : 0x01) | 0b00111010; // pertahankan setting pin/time dari setDefaults()
  writeReg(TDA7419_REG_SOFT, b);
}

void TDA7419::setMixEnable(bool leftOn, bool rightOn, bool enable) {
  uint8_t b = 0;
  if (leftOn)  b |= 0x01;
  if (rightOn) b |= 0x02;
  if (enable)  b |= 0x04;
  writeReg(TDA7419_REG_MIX_GAIN, b);
}
