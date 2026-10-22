// v34: THE EDUCATIONAL PLAYER (Marko, 18.10.2026: "take the songs from my set ... one instrumental song ... one vocal
// song ... adjust the educational player so it functions as an educational player actually"). Two whole songs
// (assets/edu/edu.json): the waveform with the sections under it, previous / next section, LOOP the section, the bar and
// beat count inside the section, NOW and NEXT, what happens in this section of this song and what to listen for now.
(() => {
  const E = document.getElementById("edu"); if(!E) return;
  const $ = id => document.getElementById(id);
  const COL = {INTRO:"--s-intro", BREAKDOWN:"--s-break", "BUILD-UP":"--s-build", DROP:"--s-drop", MAIN:"--s-main", OUTRO:"--s-outro",
    VERSE:"--s-verse", "PRE-CHORUS":"--s-pre", CHORUS:"--s-chorus", BRIDGE:"--s-bridge", INSTRUMENTAL:"--s-main"};
  const css = v => getComputedStyle(document.documentElement).getPropertyValue(v).trim() || "#888";
  const col = n => css(COL[n] || "--s-amb");
  const mmss = t => Math.floor(t/60) + ":" + String(Math.floor(t%60)).padStart(2, "0");
  const tx = k => { const el = document.querySelector('#eDict [data-i18n="' + k + '"]'); return el ? el.textContent : ""; };
  const audio = new Audio(); audio.preload = "metadata";
  let SONGS = [], S = null, W = null, ST = null, BT = null, DN = null, loop = false, raf = 0;
  // v36: the stems, Ableton Live's separation: one lane each under the waveform (Marko: "four waveforms ... running
  // together, plus a whole waveform. So the user can see how the instruments are changing through time")
  const STEM = [["VOCALS", "--s-verse"], ["DRUMS", "--s-build"], ["BASS", "--d-low"], ["OTHERS", "--d-high"]], sc = $("eSt");
  const RATE = 25, wc = $("eWv");
  const below = (a, t) => { let lo = 0, hi = a.length - 1, r = -1; while(lo <= hi){ const m = (lo + hi) >> 1; if(a[m] <= t){ r = m; lo = m + 1; } else hi = m - 1; } return r; };
  fetch("assets/edu/edu.json").then(r => r.json()).then(j => { SONGS = j.songs; load(0); });
  function load(i){
    if(!SONGS[i] || S === SONGS[i]) return;
    audio.pause(); S = SONGS[i]; W = null; loop = false; pend = null; $("eLoop").setAttribute("aria-pressed", "false");
    attach(S.src);
    BT = Float64Array.from(S.beats, b => b[0]); DN = Float64Array.from(S.beats.filter(b => b[1] === 1), b => b[0]);
    S.secT = S.sections.map(x => x[0]);
    $("eT").textContent = S.title; $("eA").textContent = S.artist; $("eCr").textContent = "Credits · " + (S.credits || S.title + " · " + S.artist); $("eC").textContent = S.code; $("eBpm").textContent = S.bpm;
    $("eDu").textContent = mmss(S.dur);
    const ch = $("eChips"); ch.innerHTML = "";
    S.sections.forEach(([t, k], n) => { const b = document.createElement("button"); b.type = "button"; b.style.setProperty("--c", col(k));
      b.textContent = k + " · " + mmss(t); b.addEventListener("click", () => go(n, true)); ch.appendChild(b); });
    fetch(S.wave).then(r => r.arrayBuffer()).then(b => { W = new Uint8Array(b); paint(); });
    ST = null; if(S.stems) fetch(S.stems).then(r => r.arrayBuffer()).then(b => { ST = new Uint8Array(b); paint(); });
    paint();
  }
  const secAt = t => Math.max(0, below(S.secT, t + 1e-3));
  const secEnd = n => n + 1 < S.sections.length ? S.sections[n+1][0] : S.dur;
  async function play(){ try{ await audio.play(); }catch(e){} }
  // a jump before the song is loaded waits for it (the browser drops a currentTime set too early)
  let pend = null;
  const seekTo = t => { if(audio.readyState > 0) audio.currentTime = t; else pend = t; };
  audio.addEventListener("loadedmetadata", () => { if(pend != null){ audio.currentTime = pend; pend = null; } });
  const now = () => pend != null ? pend : (audio.currentTime || 0);
  // HLS as the mix (10 s segments): Cloudflare Pages answers no byte ranges, so a single file could not be jumped in.
  // hls.js where the browser needs it (the mix player's copy, loaded once), Safari's own HLS otherwise.
  let hls = null;
  function attach(src){
    if(hls){ hls.destroy(); hls = null; }
    if(audio.canPlayType("application/vnd.apple.mpegurl")){ audio.src = src; return; }
    const go2 = () => { if(window.Hls && Hls.isSupported()){ hls = new Hls({maxBufferLength:30}); hls.loadSource(src); hls.attachMedia(audio); } else audio.src = src; };
    if(window.Hls) go2();
    else { const sc = document.createElement("script"); sc.src = "https://cdnjs.cloudflare.com/ajax/libs/hls.js/1.6.16/hls.min.js"; sc.onload = go2; sc.onerror = () => { audio.src = src; }; document.head.appendChild(sc); }
  }
  function go(n, start){ n = Math.max(0, Math.min(S.sections.length - 1, n)); loopSec = n; seekTo(S.sections[n][0] + .02); if(start && audio.paused) play(); paint(); }
  $("ePlay").addEventListener("click", () => audio.paused ? play() : audio.pause());
  $("ePrev").addEventListener("click", () => { const n = secAt(now()); go(now() - S.sections[n][0] > 2 ? n : n - 1); });
  $("eNext").addEventListener("click", () => go(secAt(now()) + 1));
  // LOOP holds the section it was pressed in (or the one chosen while it is on), and only acts when playback has just
  // crossed that section's end: a time read during a seek must not move it
  let loopSec = 0;
  $("eLoop").addEventListener("click", () => { loop = !loop; loopSec = secAt(now()); $("eLoop").setAttribute("aria-pressed", loop ? "true" : "false"); });
  sc.addEventListener("click", e => { const r = sc.getBoundingClientRect(), t = (e.clientX - r.left)/r.width*S.dur; loopSec = secAt(t); seekTo(t); paint(); });
  wc.addEventListener("click", e => { const r = wc.getBoundingClientRect(), t = (e.clientX - r.left)/r.width*S.dur; loopSec = secAt(t); seekTo(t); paint(); });
  audio.addEventListener("play", () => { $("ePlay").textContent = "❚❚"; $("ePlay").setAttribute("aria-label", "Pause");
    document.dispatchEvent(new CustomEvent("mantra-play", {detail:"edu"})); tick(); });
  audio.addEventListener("pause", () => { $("ePlay").textContent = "▶"; $("ePlay").setAttribute("aria-label", "Play"); paint(); });
  audio.addEventListener("seeked", paint);
  document.addEventListener("mantra-play", e => { if(e.detail !== "edu" && !audio.paused) audio.pause(); });
  // the tabs choose the song: instrumental, vocal
  document.querySelectorAll(".guide .tabs button").forEach(b => b.addEventListener("click", () => load(b.dataset.part === "gV" ? 1 : 0)));
  document.querySelectorAll(".lang button").forEach(b => b.addEventListener("click", () => setTimeout(paint, 50)));
  function tick(){ cancelAnimationFrame(raf); const f = () => {
      if(loop && !audio.seeking){ const e = secEnd(loopSec), t = now(); if(t >= e - .03 && t < e + 1.5) seekTo(S.sections[loopSec][0] + .02); }
      paint(); if(!audio.paused) raf = requestAnimationFrame(f); }; raf = requestAnimationFrame(f); }
  function fit(c){ const r = c.getBoundingClientRect(), d = Math.min(devicePixelRatio || 1, 2), w = Math.round(r.width*d), h = Math.round(r.height*d);
    if(c.width !== w || c.height !== h){ c.width = w; c.height = h; } return [w, h, d]; }
  let lastSec = -1;
  function paint(){
    if(!S) return;
    const t = now(), n = secAt(t), [k, name] = [S.sections[n][3], S.sections[n][1]];
    // the waveform, played part dimmed, the sections under it, the current one brighter
    const [w, h, d] = fit(wc), g = wc.getContext("2d"), strip = 24*d, m = (h - strip)/2, at = x => x/S.dur*w;
    g.clearRect(0, 0, w, h);
    if(W){ const low = css("--d-low"), mid = css("--d-mid"), high = css("--d-high"), step = 2*d;
      for(let x = 0; x < w; x += step){ const s = x/w*S.dur, i = Math.floor(s*RATE)*4; if(i + 2 >= W.length) break;
        g.globalAlpha = s < t ? .55 : 1;
        g.fillStyle = low;  let a = W[i]/255*m*.94;   g.fillRect(x, m - a, step - d*.5, a*2);
        g.fillStyle = mid;  a = W[i+1]/255*m*.94*.75; g.fillRect(x, m - a, step - d*.5, a*2);
        g.fillStyle = high; a = W[i+2]/255*m*.94*.5;  g.fillRect(x, m - a, step - d*.5, a*2); }
      g.globalAlpha = 1; }
    // bar lines every 8 bars, faint
    g.fillStyle = "rgba(238,240,255,.10)"; for(let q = 0; q < DN.length; q += 8){ g.fillRect(at(DN[q]), 0, d, h - strip); }
    g.font = "500 " + (11*d) + "px 'IBM Plex Mono', monospace"; g.textBaseline = "middle";
    S.sections.forEach(([s, nm], q) => { const x0 = at(s), x1 = at(secEnd(q));
      g.fillStyle = col(nm); g.globalAlpha = q === n ? 1 : .55; g.fillRect(x0, h - strip, Math.max(1, x1 - x0 - 2*d), strip);
      g.globalAlpha = .6; g.fillRect(x0, 0, 1.5*d, h - strip); g.globalAlpha = 1;
      if(x1 - x0 > g.measureText(nm).width + 10*d){ g.fillStyle = "#14121c"; g.fillText(nm, x0 + 5*d, h - strip/2); } });
    g.fillStyle = "#fff"; g.fillRect(at(t) - d, 0, 2*d, h);
    // the four stem lanes, mirrored, the played part dimmed, the section lines through them
    { const [sw, sh, sd] = fit(sc), q = sc.getContext("2d"), lane = sh/4, ats = x => x/S.dur*sw;
      q.clearRect(0, 0, sw, sh); q.font = "500 " + (9.5*sd) + "px 'IBM Plex Mono', monospace"; q.textBaseline = "top";
      STEM.forEach(([nm, cv], k) => { const y0 = k*lane, mid = y0 + lane/2;
        q.fillStyle = "rgba(238,240,255,.04)"; q.fillRect(0, y0 + sd, sw, lane - 2*sd);
        if(ST){ q.fillStyle = css(cv); const step = 2*sd;
          for(let x = 0; x < sw; x += step){ const s = x/sw*S.dur, i = Math.floor(s*RATE)*4 + k; if(i >= ST.length) break;
            q.globalAlpha = s < t ? .5 : 1; const a = ST[i]/255*(lane/2 - 3*sd); q.fillRect(x, mid - a, step - sd*.5, Math.max(sd*.6, a*2)); }
          q.globalAlpha = 1; }
        const lw = q.measureText(nm).width + 8*sd; q.fillStyle = "rgba(7,11,28,.82)"; q.fillRect(2*sd, y0 + 2*sd, lw, 13*sd);
        q.fillStyle = css(cv); q.fillText(nm, 6*sd, y0 + 4*sd); });
      q.fillStyle = "rgba(238,240,255,.18)"; S.sections.forEach(([s0]) => q.fillRect(ats(s0), 0, sd, sh));
      q.fillStyle = "#fff"; q.fillRect(ats(t) - sd, 0, 2*sd, sh); }
    // which stems sound now: the average of the last half second over a fixed line
    if(ST){ const f0 = Math.max(0, Math.floor((t - .5)*RATE)), f1 = Math.max(f0 + 1, Math.floor(t*RATE) + 1), on = [];
      STEM.forEach(([nm, cv], k) => { let a = 0, n = 0; for(let f = f0; f < f1; f++){ const i = f*4 + k; if(i < ST.length){ a += ST[i]; n++; } }
        if(n && a/n > 40) on.push('<i style="--c:' + css(cv) + '">' + nm + '</i>'); });
      $("eIn").innerHTML = "IN · " + (on.length ? on.join(" · ") : "—"); }
    // NOW, the count, NEXT
    E.querySelector(".cur").style.setProperty("--c", col(name)); E.querySelector(".teach").style.setProperty("--c", col(name));
    $("eNow").textContent = name;
    const total = S.sections[n][2], bi = below(DN, t + 1e-3), b0 = below(DN, S.sections[n][0] + 1e-3), barIn = Math.max(1, bi - b0 + 1);
    $("eBar").textContent = "BAR " + Math.min(barIn, total) + " / " + total;
    const k2 = below(BT, t + 1e-3), beat = k2 >= 0 ? S.beats[k2][1] - 1 : -1;
    [...$("eBeats").children].forEach((e, q) => { e.classList.toggle("on", q === beat); e.classList.toggle("one", q === 0 && beat === 0); });
    const ph = (barIn - 1) % 8; [...$("ePhr").children].forEach((e, q) => { e.classList.toggle("on", q === ph); e.classList.toggle("done", q < ph); });
    E.querySelector(".nxt").style.setProperty("--c", n + 1 < S.sections.length ? col(S.sections[n+1][1]) : "#9AA3C7");
    if(n + 1 < S.sections.length){ const nt = S.sections[n+1][0], bars = below(DN, nt - .05) - bi;
      $("eNx").textContent = loop ? "LOOP" : S.sections[n+1][1]; $("eNt").textContent = loop ? mmss(secEnd(n) - t) : (bars > 0 ? bars + (bars === 1 ? " bar" : " bars") + " · " : "") + mmss(nt - t); }
    else { $("eNx").textContent = "END"; $("eNt").textContent = mmss(S.dur - t); }
    $("eEl").textContent = mmss(t);
    // the teaching: this section of this song, and the last thing that happened
    $("eTxt").textContent = tx("edu." + S.id + "." + k);
    const ev = S.events.filter(x => x[0] <= t + .05).pop(); $("eEv").textContent = ev ? tx(ev[1]) : "—";
    if(n !== lastSec){ lastSec = n; [...$("eChips").children].forEach((c, q) => c.classList.toggle("on", q === n)); }
  }
  new ResizeObserver(paint).observe(E);
})();
