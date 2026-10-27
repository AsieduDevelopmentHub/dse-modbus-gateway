#pragma once

// Embedded local overview page (PROGMEM). Served at GET /.

#include <Arduino.h>

static const char OVERVIEW_HTML[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1"/>
<title>DSE Gateway</title>
<style>
:root{
  --bg:#0e1412;--bg2:#16201c;--ink:#e8f0eb;--muted:#8aa396;
  --accent:#3d9b6e;--warn:#d4a017;--bad:#c44b4b;--line:#24332c;
  --font:"Segoe UI",system-ui,sans-serif;
}
*{box-sizing:border-box}
body{margin:0;font-family:var(--font);background:radial-gradient(1200px 600px at 10% -10%,#1a2e26 0%,var(--bg) 55%);
color:var(--ink);min-height:100vh}
.wrap{max-width:720px;margin:0 auto;padding:1.25rem 1rem 2.5rem}
.brand{font-size:clamp(1.6rem,5vw,2.2rem);font-weight:700;letter-spacing:.02em;margin:0}
.sub{color:var(--muted);margin:.35rem 0 1.25rem;font-size:.95rem}
.hero{padding:1.25rem 0 1rem;border-bottom:1px solid var(--line)}
.headline{font-size:clamp(1.8rem,6vw,2.6rem);font-weight:650;margin:.2rem 0;line-height:1.1}
.meta{color:var(--muted);font-size:.85rem}
.pill{display:inline-block;padding:.15rem .55rem;border-radius:4px;font-size:.75rem;
background:var(--bg2);border:1px solid var(--line);margin-right:.35rem}
.pill.ok{border-color:var(--accent);color:#9fd9b8}
.pill.bad{border-color:var(--bad);color:#f0a8a8}
.pill.warn{border-color:var(--warn);color:#e8d08a}
.grid{display:grid;grid-template-columns:repeat(2,1fr);gap:.65rem;margin-top:1.1rem}
@media(min-width:560px){.grid{grid-template-columns:repeat(3,1fr)}}
.metric{background:var(--bg2);border:1px solid var(--line);padding:.75rem .85rem}
.metric .label{color:var(--muted);font-size:.72rem;text-transform:uppercase;letter-spacing:.06em}
.metric .val{font-size:1.35rem;font-weight:600;margin-top:.2rem;font-variant-numeric:tabular-nums}
.metric .unit{color:var(--muted);font-size:.8rem;font-weight:400}
.section{margin-top:1.4rem}
.section h2{font-size:.8rem;text-transform:uppercase;letter-spacing:.08em;color:var(--muted);
margin:0 0 .55rem;font-weight:600}
.alarms{list-style:none;padding:0;margin:0}
.alarms li{padding:.55rem .7rem;border:1px solid var(--line);background:var(--bg2);margin-bottom:.4rem;
font-size:.9rem}
.alarms .st{color:var(--bad);font-weight:600}
.empty{color:var(--muted);font-size:.9rem}
.stale{opacity:.55}
footer{margin-top:2rem;color:var(--muted);font-size:.75rem}
</style>
</head>
<body>
<div class="wrap" id="app">
  <header class="hero">
    <p class="brand">DSE Gateway</p>
    <p class="sub" id="support">Connecting…</p>
    <div class="headline" id="headline">—</div>
    <p class="meta" id="meta"></p>
  </header>
  <div class="grid" id="metrics"></div>
  <section class="section">
    <h2>Active alarms</h2>
    <ul class="alarms" id="alarms"><li class="empty">Waiting for data…</li></ul>
  </section>
  <footer>Polls /api/status every 2s · LAN/AP only · no auth</footer>
</div>
<script>
const $=id=>document.getElementById(id);
function fmt(v,d){return v==null||v===undefined?'—':Number(v).toFixed(d)}
function pill(link){
  if(link==='ok')return '<span class="pill ok">link ok</span>';
  if(link==='degraded')return '<span class="pill warn">degraded</span>';
  if(link==='fail')return '<span class="pill bad">link fail</span>';
  return '<span class="pill">link ?</span>';
}
function metric(label,val,unit){
  return `<div class="metric"><div class="label">${label}</div><div class="val">${val}`+
    (unit?` <span class="unit">${unit}</span>`:'')+`</div></div>`;
}
function render(s){
  const stale=s.ageMs>8000;
  $('app').classList.toggle('stale',stale);
  $('headline').textContent=s.headline||'—';
  const mode=s.opMode?s.opMode.name:'—';
  const eng=s.engStatus?s.engStatus.name:'—';
  $('support').innerHTML=`${mode} · ${eng} · ${pill(s.link)}`+
    (stale?' <span class="pill warn">stale</span>':'');
  $('meta').textContent=`slave ${s.slaveId} · ${s.wifiMode} ${s.wifiIp}`+
    (s.rssi?` · ${s.rssi} dBm`:'')+` · age ${s.ageMs||0} ms`+
    (s.lastError?` · ${s.lastError}`:'');
  $('metrics').innerHTML=[
    metric('Battery',fmt(s.batteryV,1),'V'),
    metric('Engine RPM',fmt(s.engRpm,0),''),
    metric('Frequency',fmt(s.genFreq,1),'Hz'),
    metric('L1 voltage',fmt(s.genVan,1),'V'),
    metric('L2 voltage',fmt(s.genVbn,1),'V'),
    metric('L3 voltage',fmt(s.genVcn,1),'V'),
    metric('L1 current',fmt(s.genIa,1),'A'),
    metric('L2 current',fmt(s.genIb,1),'A'),
    metric('L3 current',fmt(s.genIc,1),'A'),
    metric('Total power',fmt(s.genPTotKw,2),'kW'),
    metric('Load',fmt(s.genPctKw,1),'%'),
  ].join('');
  const al=s.alarms||[];
  if(!al.length){
    $('alarms').innerHTML='<li class="empty">No active alarms</li>';
  }else{
    $('alarms').innerHTML=al.map(a=>
      `<li><span class="st">${a.state}</span> — ${a.name} <span class="meta">@${a.addr}[${a.slot}]</span></li>`
    ).join('');
  }
}
async function tick(){
  try{
    const r=await fetch('/api/status');
    if(!r.ok)throw new Error('HTTP '+r.status);
    render(await r.json());
  }catch(e){
    $('headline').textContent='Offline';
    $('support').textContent='Cannot reach /api/status';
    $('app').classList.add('stale');
  }
}
tick();
setInterval(tick,2000);
</script>
</body>
</html>)HTML";
