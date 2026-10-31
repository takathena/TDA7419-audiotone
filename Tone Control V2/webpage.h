// webpage.h
// Halaman kontrol yang di-serve langsung dari ESP32 (tidak butuh internet/app store).
#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <Arduino.h>

const char MAIN_PAGE[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<title>TDA7419 Tone Control</title>
<style>
  :root{
    --bg:#0f1115; --card:#181b22; --accent:#4f9dff; --accent2:#ff5f6d;
    --text:#eef1f6; --muted:#8a93a6; --track:#2a2f3a;
  }
  *{box-sizing:border-box;}
  body{
    margin:0; padding:16px; padding-bottom:40px;
    background:var(--bg); color:var(--text);
    font-family:-apple-system,Segoe UI,Roboto,Helvetica,Arial,sans-serif;
  }
  h1{font-size:1.15rem; font-weight:600; margin:0 0 4px;}
  .sub{color:var(--muted); font-size:0.8rem; margin-bottom:16px;}
  .card{
    background:var(--card); border-radius:14px; padding:16px;
    margin-bottom:12px; box-shadow:0 2px 10px rgba(0,0,0,.25);
  }
  .row{display:flex; justify-content:space-between; align-items:center; margin-bottom:6px;}
  .row label{font-size:0.9rem; font-weight:500;}
  .row .val{font-variant-numeric:tabular-nums; color:var(--accent); font-weight:600; font-size:0.9rem;}
  input[type=range]{
    -webkit-appearance:none; width:100%; height:6px; border-radius:4px;
    background:var(--track); outline:none;
  }
  input[type=range]::-webkit-slider-thumb{
    -webkit-appearance:none; width:22px; height:22px; border-radius:50%;
    background:var(--accent); cursor:pointer; border:3px solid #0f1115;
  }
  input[type=range]::-moz-range-thumb{
    width:22px; height:22px; border-radius:50%; background:var(--accent); border:3px solid #0f1115; cursor:pointer;
  }
  select{
    width:100%; padding:10px; border-radius:10px; border:1px solid var(--track);
    background:var(--track); color:var(--text); font-size:0.95rem;
  }
  .grid2{display:grid; grid-template-columns:1fr 1fr; gap:12px;}
  .btn{
    width:100%; padding:12px; border-radius:12px; border:none; font-size:1rem; font-weight:600;
    cursor:pointer; transition:.15s;
  }
  .btn-mute{background:var(--track); color:var(--text);}
  .btn-mute.active{background:var(--accent2); color:#1a0a0b;}
  .btn-loud{background:var(--track); color:var(--text);}
  .btn-loud.active{background:var(--accent); color:#071021;}
  .status{font-size:0.75rem; color:var(--muted); text-align:center; margin-top:10px;}
  .dot{display:inline-block; width:8px; height:8px; border-radius:50%; background:#2ecc71; margin-right:6px;}
  .dot.off{background:#e74c3c;}
  .bt-card .row{margin-bottom:2px;}
  .bt-track{font-size:0.8rem; color:var(--muted); margin-top:4px;}
  .bt-hint{font-size:0.72rem; color:var(--muted); margin-top:8px; line-height:1.4;}
</style>
</head>
<body>
  <h1>TDA7419 Tone Control</h1>
  <div class="sub">KHOBE CREATIVE &middot; Digital AudioTone Control WiFi</div>

  <div class="card">
    <div class="row"><label>Sumber Input</label></div>
    <select id="input" onchange="sendParam('input', this.value)">
      <option value="0">Input QD (differential)</option>
      <option value="1" selected>Input 1 (SE1)</option>
      <option value="2">Input 2 (SE2)</option>
      <option value="3">Input 3 (SE3)</option>
      <option value="4">Bluetooth (A2DP)</option>
    </select>
  </div>

  <div class="card bt-card">
    <div class="row"><label>Bluetooth</label><span class="dot" id="btDot"></span></div>
    <div class="bt-track" id="btStatus">Menunggu koneksi...</div>
    <div class="bt-track" id="btTrack"></div>
    <div class="bt-hint">Pairing HP ke Bluetooth bernama sama dengan WiFi-nya, lalu pilih
    "Bluetooth (A2DP)" di Sumber Input di atas supaya suaranya terdengar.</div>
  </div>

  <div class="card">
    <div class="grid2">
      <button class="btn btn-mute" id="btnMute" onclick="toggleMute()">Mute</button>
      <button class="btn btn-loud" id="btnLoud" onclick="toggleLoudness()">Loudness</button>
    </div>
  </div>

  <div class="card">
    <div class="row"><label>Volume</label><span class="val" id="vVolume">0 dB</span></div>
    <input type="range" id="volume" min="-79" max="15" value="-30" oninput="live('volume',this.value,'dB')" onchange="sendParam('volume',this.value)">
  </div>

  <div class="card">
    <div class="row"><label>Balance (Kiri &harr; Kanan)</label><span class="val" id="vBalance">0</span></div>
    <input type="range" id="balance" min="-40" max="40" value="0" oninput="live('balance',this.value)" onchange="sendParam('balance',this.value)">
  </div>

  <div class="card">
    <div class="row"><label>Fader (Depan &harr; Belakang)</label><span class="val" id="vFader">0</span></div>
    <input type="range" id="fader" min="-40" max="40" value="0" oninput="live('fader',this.value)" onchange="sendParam('fader',this.value)">
  </div>

  <div class="card">
    <div class="row"><label>Bass</label><span class="val" id="vBass">0 dB</span></div>
    <input type="range" id="bass" min="-15" max="15" value="0" oninput="live('bass',this.value,'dB')" onchange="sendParam('bass',this.value)">
  </div>

  <div class="card">
    <div class="row"><label>Middle</label><span class="val" id="vMiddle">0 dB</span></div>
    <input type="range" id="middle" min="-15" max="15" value="0" oninput="live('middle',this.value,'dB')" onchange="sendParam('middle',this.value)">
  </div>

  <div class="card">
    <div class="row"><label>Treble</label><span class="val" id="vTreble">0 dB</span></div>
    <input type="range" id="treble" min="-15" max="15" value="0" oninput="live('treble',this.value,'dB')" onchange="sendParam('treble',this.value)">
  </div>

  <div class="card">
    <div class="row"><label>Subwoofer</label><span class="val" id="vSub">0 dB</span></div>
    <input type="range" id="sub" min="-79" max="15" value="0" oninput="live('sub',this.value,'dB')" onchange="sendParam('sub',this.value)">
  </div>

  <div class="status"><span class="dot" id="statusDot"></span><span id="statusText">Terhubung</span></div>

<script>
let muted = false, loudOn = false;

function live(id, v, suffix){
  document.getElementById('v' + id.charAt(0).toUpperCase() + id.slice(1)).innerText = v + (suffix ? ' ' + suffix : '');
}

function sendParam(key, value){
  fetch('/api/set?' + key + '=' + value)
    .then(r => r.text())
    .then(() => setStatus(true))
    .catch(() => setStatus(false));
}

function toggleMute(){
  muted = !muted;
  document.getElementById('btnMute').classList.toggle('active', muted);
  sendParam('mute', muted ? 1 : 0);
}

function toggleLoudness(){
  loudOn = !loudOn;
  document.getElementById('btnLoud').classList.toggle('active', loudOn);
  sendParam('loud', loudOn ? 1 : 0);
}

function setStatus(ok){
  document.getElementById('statusDot').classList.toggle('off', !ok);
  document.getElementById('statusText').innerText = ok ? 'Terhubung' : 'Gagal kirim, cek koneksi';
}

function loadState(){
  fetch('/api/state').then(r => r.json()).then(s => {
    document.getElementById('volume').value = s.volume;  live('volume', s.volume, 'dB');
    document.getElementById('bass').value = s.bass;       live('bass', s.bass, 'dB');
    document.getElementById('middle').value = s.middle;   live('middle', s.middle, 'dB');
    document.getElementById('treble').value = s.treble;   live('treble', s.treble, 'dB');
    document.getElementById('balance').value = s.balance; live('balance', s.balance);
    document.getElementById('fader').value = s.fader;     live('fader', s.fader);
    document.getElementById('sub').value = s.sub;         live('sub', s.sub, 'dB');
    document.getElementById('input').value = s.input;
    muted = s.mute; document.getElementById('btnMute').classList.toggle('active', muted);
    loudOn = s.loud; document.getElementById('btnLoud').classList.toggle('active', loudOn);

    document.getElementById('btDot').classList.toggle('off', !s.bt_connected);
    document.getElementById('btStatus').innerText = s.bt_connected ? 'Terhubung' : 'Menunggu koneksi...';
    document.getElementById('btTrack').innerText = (s.bt_title || s.bt_artist)
      ? (s.bt_title + (s.bt_artist ? ' — ' + s.bt_artist : ''))
      : '';
  }).catch(() => setStatus(false));
}

window.onload = loadState;
setInterval(loadState, 4000); // refresh status BT & judul lagu tiap 4 detik
</script>
</body>
</html>
)HTML";

#endif // WEBPAGE_H
