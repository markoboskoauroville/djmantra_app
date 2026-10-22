#!/usr/bin/env python3
"""
Builds site/public/index.html, the page that sells DJ Mantra (Marko, 22.10.2026: "build the website to sell my DJ
app ... its strong side is that it trains you to understand beats and song sections ... get this lesson about beats
and song sections for my DJ Mantra web page and transfer it to this page ... more like educational app ... all its
features are promoted").

    python3 site/build.py

The lesson's words come from djmantra.pages.dev (DJMANTRA_ECSTATIC_DANCE, its English dictionary) through
lesson_en.json, and its educational player is that page's (edu_player.js, guide.css, public/assets/edu), so the two
pages teach the same thing in the same words. Requests: momentaryupdates.md, 22.10.2026.
"""
import html
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
L = json.loads((HERE / "lesson_en.json").read_text())
GUIDE_CSS = (HERE / "guide.css").read_text()
PLAYER_JS = (HERE / "edu_player.js").read_text()

# The tag only once djmantraapp.pages.dev is registered in Amazon PartnerNet (MANTRA_MANIFEST payment-links.md:
# a new site is registered before it carries affiliate links). Set REGISTERED = True then.
REGISTERED = True
AMAZON = "https://www.amazon.de/dp/B0DMTBMS7D" + ("?tag=markomantresh-21" if REGISTERED else "")
# TIDAL's programme runs on Awin (TIDAL Global). Until Marko is approved there, the plain link; then his Awin link.
TIDAL = "https://tidal.com/"
DOWNLOAD = "https://github.com/markoboskoauroville/djmantra_app/releases/tag/android-latest"
REVOLUT = "https://revolut.me/mbosko0612"


def t(key: str) -> str:
    """A lesson string as it is on djmantra.pages.dev, escaped (the few with markup are marked *_html)."""
    return html.escape(L[key])


def raw(key: str) -> str:
    return L[key]


cells = "".join(
    f'<i class="{"one" if i % 4 == 0 else ""}{" gap" if i % 4 == 3 else ""}"></i>' for i in range(32)
)

SECTIONS_I = [
    ("INTRO", "16 / 32", "intro"), ("BREAKDOWN", "8 / 16 / 32", "break"), ("BUILD-UP", "4 / 8", "build"),
    ("DROP", "1", "drop"), ("MAIN", "16 / 32 / 64", "main"), ("OUTRO", "16 / 32", "outro"),
]
SECTIONS_V = [
    ("INTRO", "8 / 16 / 32", "intro"), ("VERSE", "8 / 16", "verse"), ("PRE-CHORUS", "4 / 8", "pre"),
    ("CHORUS", "8 / 16", "chorus"), ("BRIDGE", "4 / 8", "bridge"), ("OUTRO", "8 / 16 / 32", "outro"),
]


def sec_cards(items, prefix, with_wave):
    out = []
    for name, bars, k in items:
        body = f'<p>{t(f"{prefix}.{k}")}</p>'
        if with_wave and f"{prefix}.{k}.w" in L:
            body += f'<p>{t(f"{prefix}.{k}.w")}</p>'
        body += f'<p class="dj">{t(f"{prefix}.{k}.dj")}</p>'
        out.append(f'<div class="sc" data-s="{name}"><h3>{name}<small>{bars}</small></h3>{body}</div>')
    return "\n".join(out)


EVENTS = {"e.kick", "e.hats", "e.stop", "e.rise", "e.bascut", "e.drop", "e.out", "e.end", "e.voice", "e.stopv", "e.hook",
          "e.novoice", "e.story", "e.echo", "e.last", "e.noout"}
edu_dict = "".join(f'<span data-i18n="{k}">{t(k)}</span>' for k in L
                   if (k.startswith("edu.") and k != "edu.h") or k in EVENTS)

FEATURES = [
    ("Phrasing view", "The pads count the phrase: beat 1 to 8 lights as the song plays, so you bring the next song in on the one. NOW and NEXT name the section you are in and the one coming."),
    ("Beat lights on the controller", "On the Hercules DJControl Mix Ultra the pads light the beats in every pad mode. SHIFT + pad 1 says “1 is here”."),
    ("Harmonic keys", "Every song's key in Camelot notation; the top bar warns when two keys clash (10A > 7B) and the Keys tab finds what fits."),
    ("Stems in the mixer", "Turn down the vocals, the drums or the harmony of a song while it plays: the classic way to mix two singers without a clash."),
    ("AutoMix, Tempo Blend, Auto Phase", "Let the app mix a playlist for you and watch how it lines up beats and phrases, then do it yourself."),
    ("Waveforms in three colours", "Red bass and drums, green mids, blue highs, as in the lesson: you see the breakdown and the drop coming."),
    ("Mixxx engine", "The open-source Mixxx 2.5.6 engine: two decks, sync, key lock, pitch, loops, hot cues, EQs, effects, BPM and key analysis."),
    ("USB sticks that keep playing", "Songs from USB drives and SD cards are copied before they play, so pulling the stick out does not stop the music."),
    ("Folders like in djay", "Add a folder and it becomes a tree of playlists that stays in the library, even with the drive unplugged."),
    ("A library for DJs", "★ ratings, every column sortable, an Export column, and lists of missing songs with links to TIDAL, Bandcamp, Beatport and Qobuz."),
    ("Video files as tracks", "MP4, MOV, MKV and more play their sound; the first picture becomes the cover."),
    ("Sound anywhere", "The phone's speaker, USB audio or Bluetooth, following you when headphones or a speaker are connected."),
]
features_html = "\n".join(f'<div class="card"><h3>{html.escape(h)}</h3><p>{html.escape(p)}</p></div>' for h, p in FEATURES)

BOWL = (HERE.parent.parent / ".." / "MANTRA_MANIFEST").resolve()
BOWL = (Path.home() / "Developer/MANTRA_MANIFEST/kits/donation-bowl/donation-bowl.html").read_text()

PAGE = f"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="icon" type="image/svg+xml" href="/favicon.svg">
<link rel="apple-touch-icon" href="/apple-touch-icon.png">
<title>DJ Mantra · learn to DJ by ear</title>
<meta name="awin" content="Awin: this site is run by Marko Boško, publisher of DJ Mantra">
<meta name="description" content="DJ Mantra, the Android DJ app that trains you to hear beats, phrases and song sections. With the Hercules DJControl Mix Ultra.">
<link rel="preconnect" href="https://fonts.googleapis.com">
<link href="https://fonts.googleapis.com/css2?family=IBM+Plex+Mono:wght@400;500&family=Inter:wght@400;500;600;700&family=Spectral:wght@400&display=swap" rel="stylesheet">
<style>
:root{{--bg:#070B1C; --bg2:#0B1026; --panel:#151C3D; --tile:#1F2850; --ink:#EEF0FF; --ink2:#9AA3C7; --ink3:#6E77A0;
  --amber:#FFD54A; --line:rgba(255,213,74,.25); --line2:rgba(255,255,255,.08);
  --d-low:#FF6B5E; --d-mid:#6CCB72; --d-high:#5AA2F0; --d-blue:#FFD54A;
  --s-amb:#6b6478; --s-break:#9f7ae0; --s-bridge:#9f7ae0; --s-build:#e9c84a; --s-chorus:#ff3d7a; --s-drop:#ff3d7a;
  --s-intro:#4fc3c7; --s-main:#ff8a2b; --s-outro:#38b6ec; --s-verse:#6CCB72; --s-pre:#e9c84a;}}
*{{box-sizing:border-box}}
html{{scroll-behavior:smooth}}
body{{margin:0; background:var(--bg); color:var(--ink); font:16px/1.6 Inter,system-ui,sans-serif; overflow-x:hidden}}
a{{color:var(--amber)}}
.wrap{{max-width:1080px; margin:0 auto; padding:0 16px}}
section{{padding:64px 0}}
.eyebrow{{font:500 12px "IBM Plex Mono",monospace; letter-spacing:.18em; text-transform:uppercase; color:var(--amber); margin-bottom:10px}}
h1{{font:400 clamp(40px,7vw,72px)/1.05 Spectral,Georgia,serif; margin:0 0 18px}}
h2.sec{{font:400 clamp(30px,4.5vw,46px)/1.15 Spectral,Georgia,serif; margin:0 0 16px}}
h2.sec em,h1 em{{font-style:normal; color:var(--amber)}}
.lede{{font-size:19px; color:var(--ink2); max-width:40em}}
.body{{color:var(--ink2)}}
.btn{{display:inline-block; font:600 16px Inter,sans-serif; text-decoration:none; color:#1A1A1A; background:var(--amber); border-radius:999px; padding:14px 26px; box-shadow:0 0 28px rgba(255,213,74,.35)}}
.btn.ghost{{background:transparent; color:var(--ink); border:1px solid var(--line); box-shadow:none}}
.links{{display:flex; gap:12px; flex-wrap:wrap; margin-top:26px}}
.hero{{padding:0 0 40px}}
.hero .shot{{display:block; width:100%; max-height:82vh; object-fit:cover; object-position:center 60%; margin:0 0 36px;
  -webkit-mask-image:linear-gradient(180deg,#000 82%,transparent); mask-image:linear-gradient(180deg,#000 82%,transparent)}}
.hero .grid{{display:block}}
.hero .grid > div{{max-width:640px}}
.pill{{display:inline-block; font:500 12px "IBM Plex Mono",monospace; letter-spacing:.1em; color:var(--ink2); border:1px solid var(--line2); border-radius:999px; padding:6px 12px; margin:0 6px 8px 0}}
.cards{{display:grid; grid-template-columns:repeat(auto-fill,minmax(250px,1fr)); gap:14px; margin-top:26px}}
.card{{background:var(--bg2); border:1px solid var(--line2); border-radius:16px; padding:20px}}
.card h3{{margin:0 0 6px; font-size:17px}}
.card p{{margin:0; color:var(--ink2); font-size:15px}}
.learn{{display:grid; grid-template-columns:repeat(3,minmax(0,1fr)); gap:18px; margin-top:30px}}
.learn figure{{margin:0; background:var(--bg2); border:1px solid var(--line2); border-radius:18px; padding:14px}}
.learn img{{width:100%; border-radius:12px; display:block}}
.learn figcaption{{font-size:14.5px; color:var(--ink2); margin-top:10px}}
.learn figcaption b{{color:var(--ink); display:block; font-size:16px}}
.gear{{display:grid; grid-template-columns:repeat(auto-fit,minmax(280px,1fr)); gap:18px; margin-top:26px}}
.gear .card{{padding:24px}}
.gear .card .btn{{margin-top:16px}}
.note{{font-size:13.5px; color:var(--ink3)}}
.get{{text-align:center; background:radial-gradient(ellipse at 50% 0%,rgba(255,213,74,.12),transparent 60%)}}
footer{{padding:40px 0 60px; color:var(--ink3); font-size:13.5px; border-top:1px solid var(--line2)}}
.guide{{background:var(--bg2)}}
.guide .ink-unit{{}}
{GUIDE_CSS}
@media (max-width:760px){{
  section{{padding:46px 0}}
  .learn{{grid-template-columns:1fr}}
  .edu{{padding:10px}} .edu canvas{{height:130px}} .edu .top code{{margin-left:0}}
  .edu .now{{grid-template-columns:1fr 1fr}} .edu .now .cnt{{grid-column:1 / -1; grid-row:2; flex-direction:row; justify-content:space-between; background:none; padding:0}}
}}
</style>
</head>
<body>

<header class="hero">
  <img class="shot" src="img/hero.jpg" alt="A night party in a Pondicherry courtyard: an international crowd dances, the DJ plays DJ Mantra on a tablet over the white Hercules DJControl Mix Ultra" width="1376" height="768">
  <div class="wrap grid">
    <div>
      <div class="eyebrow">DJ Mantra · from Auroville and Pondicherry</div>
      <h1>Learn to DJ <em>by ear</em></h1>
      <p class="lede">The DJ app that trains you to hear the music: every beat, every phrase, every intro, breakdown and drop. Mix on your phone with the Hercules DJControl Mix Ultra, and understand what you hear while you play.</p>
      <div><span class="pill">BEAT · BAR · PHRASE</span><span class="pill">SONG SECTIONS</span><span class="pill">HARMONIC KEYS</span><span class="pill">STEMS</span></div>
      <div class="links">
        <a class="btn" href="#get">Get DJ Mantra</a>
        <a class="btn ghost" href="#lesson">Try the free lesson</a>
      </div>
    </div>
  </div>
</header>

<section id="learn">
  <div class="wrap">
    <div class="eyebrow">An app that teaches</div>
    <h2 class="sec">It shows you what <em>a DJ hears</em></h2>
    <p class="lede">A DJ mixes parts of songs, not songs. DJ Mantra counts the beats and the phrases for you, names the section you are in and the one that is coming, and lights it on the controller, until you hear it without looking.</p>
    <div class="learn">
      <figure><img src="img/screen-phrasing.jpg" alt="The Phrasing view" loading="lazy"><figcaption><b>Phrasing</b>The pads light beat 1 to 8 of the phrase on both decks: bring the next song in on the one.</figcaption></figure>
      <figure><img src="img/screen-songs.jpg" alt="Two songs playing, their keys in the top bar" loading="lazy"><figcaption><b>Keys and the library</b>Each song's BPM and key; the top bar says when two keys clash, so you learn harmonic mixing as you go.</figcaption></figure>
      <figure><img src="img/screen-mixer.jpg" alt="The mixer with stems, EQ and tempo" loading="lazy"><figcaption><b>Mixer with stems</b>Harmony, vocals and drums of each song on their own knobs, the filter, a flanger, tempo ±10 %.</figcaption></figure>
    </div>
  </div>
</section>

<section id="lesson" class="guide">
  <div class="wrap">
    <div style="max-width:44em">
      <div class="eyebrow">The free lesson · {t("g.e").split("·")[-1].strip()}</div>
      <h2 class="sec">How a song is <em>built</em></h2>
      <p class="lede" style="max-width:none">{t("g.l").replace("on my demo mix", "on two songs from my set").replace("every example is a real waveform, every part can be played", "every part is a real waveform you can play")}</p>
    </div>
    <h3 style="margin:30px 0 8px">{t("g.u.h")}</h3>
    <div class="units">
      <div class="unit"><b>Beat</b><p>{t("g.u.beat")}</p></div>
      <div class="unit"><b>Bar · 4 beats</b><p>{t("g.u.bar")}</p></div>
      <div class="unit"><b>Phrase · 8 bars</b><p>{t("g.u.phr")}</p><div class="cells" aria-hidden="true">{cells}</div></div>
      <div class="unit"><b>Section · 8 / 16 / 32</b><p>{t("g.u.sec")}</p></div>
    </div>
    <p class="body" style="max-width:44em">{raw("g.wave")}</p>
    <div class="tabs" role="tablist">
      <button type="button" role="tab" aria-selected="true" data-part="gI">{t("g.t1")}</button>
      <button type="button" role="tab" aria-selected="false" data-part="gV">{t("g.t2")}</button>
    </div>
    <div class="edu" id="edu" aria-label="Educational player">
      <p class="lead">{t("edu.h")}</p>
      <div class="top"><div><b id="eT">—</b><span id="eA"></span></div><code id="eC"></code></div>
      <p class="cr" id="eCr"></p>
      <canvas id="eWv" aria-label="Waveform: click to jump"></canvas>
      <canvas id="eSt" class="stems" aria-label="The stems: vocals, drums, bass, others"></canvas>
      <div class="ctl">
        <button type="button" id="ePrev" aria-label="Previous section">◀◀</button>
        <button type="button" id="ePlay" class="play" aria-label="Play">▶</button>
        <button type="button" id="eNext" aria-label="Next section">▶▶</button>
        <button type="button" id="eLoop" class="loop" aria-pressed="false">LOOP</button>
        <span class="tm"><b id="eEl">0:00</b> / <span id="eDu">0:00</span> · <span id="eBpm"></span> BPM</span>
      </div>
      <div class="now">
        <div class="cur"><span class="lbl">NOW</span><b id="eNow">—</b><span id="eBar">—</span><span class="in" id="eIn"></span></div>
        <div class="cnt"><div class="beats" id="eBeats"><i></i><i></i><i></i><i></i></div><div class="phr" id="ePhr"><i></i><i></i><i></i><i></i><i></i><i></i><i></i><i></i></div></div>
        <div class="nxt"><span class="lbl">NEXT</span><b id="eNx">—</b><span id="eNt">—</span></div>
      </div>
      <div class="teach"><p id="eTxt"></p><p class="ev"><span class="lbl">LISTEN</span> <span id="eEv">—</span></p></div>
      <div class="chips" id="eChips"></div>
    </div>
    <div hidden id="eDict">{edu_dict}</div>
    <div class="part" id="gI">
      <p class="body" style="max-width:44em">{t("g.i.l")}</p>
      <div class="secs">
{sec_cards(SECTIONS_I, "g.i", True)}
      </div>
    </div>
    <div class="part" id="gV" hidden>
      <p class="body" style="max-width:44em">{t("g.v.l")}</p>
      <div class="secs">
{sec_cards(SECTIONS_V, "g.v", False)}
      </div>
    </div>
    <div class="gear" style="margin-top:10px">
      <div>
        <h3 class="sec" style="font:400 30px Spectral,Georgia,serif; margin:0 0 12px">{t("g.c.h")}</h3>
        <p class="body">{t("g.c.i")}</p>
        <div class="codes"><div><span>Instrumental ·</span> <b>{t("g.c.ki")}</b></div><div><span>Vocal ·</span> <b>{t("g.c.kv")}</b></div></div>
        <p class="body">{raw("g.c.ex")}</p>
      </div>
      <div>
        <h3 class="sec" style="font:400 30px Spectral,Georgia,serif; margin:0 0 12px">{t("g.m.h")}</h3>
        <p class="body">{t("g.m.i")}</p>
        <p class="body">{t("g.m.v")}</p>
      </div>
    </div>
    <p class="note" style="margin-top:18px">{t("g.src")}</p>
    <p class="body" style="margin-top:24px"><b style="color:var(--ink)">In the app, this is live:</b> the Phrasing view counts the bars and phrases of the song you are playing, and the controller's pads light the beat, so the lesson goes on while you mix.</p>
  </div>
</section>

<section id="features">
  <div class="wrap">
    <div class="eyebrow">Everything in it</div>
    <h2 class="sec">A full DJ app <em>in your phone</em></h2>
    <div class="cards">
{features_html}
    </div>
  </div>
</section>

<section id="gear">
  <div class="wrap">
    <div class="eyebrow">What you need</div>
    <h2 class="sec">A phone, a controller, <em>your music</em></h2>
    <div class="gear">
      <div class="card">
        <h3>Hercules DJControl Mix Ultra</h3>
        <p>The controller DJ Mantra is made for: two jog wheels, 16 pads that light the beats, mixer, filter, a battery for hours. It connects to the phone over Bluetooth by itself, or with a USB-C cable. Best of NAMM 2025.</p>
        <a class="btn" href="{AMAZON}" target="_blank" rel="noopener sponsored">DJControl Mix Ultra on Amazon.de →</a>
        <p class="note" style="margin:12px 0 0">About €169 on Amazon.de, October 2026.</p>
      </div>
      <div class="card">
        <h3>Your music on TIDAL</h3>
        <p>DJ Mantra plays your own files. Find the songs you are missing on TIDAL, in full quality: the library's Export lists them with a TIDAL link each.</p>
        <a class="btn" href="{TIDAL}" target="_blank" rel="noopener sponsored">Try TIDAL →</a>
      </div>
    </div>
  </div>
</section>

<section id="get" class="get">
  <div class="wrap">
    <div class="eyebrow">Get DJ Mantra</div>
    <h2 class="sec">Start <em>today</em></h2>
    <p class="lede" style="margin:0 auto 26px">For Android phones (64-bit, Android 8 or newer). Install the app, connect the Mix Ultra, and open the Phrasing view with your first song.</p>
    <a class="btn" href="{DOWNLOAD}" target="_blank" rel="noopener">Download the latest version</a>
    <p class="note" style="margin-top:14px">Free to download, open source, built on Mixxx (GPL). Install the APK and allow installing from this source when the phone asks. If it helps you learn, pay what you want below.</p>
  </div>
</section>

{BOWL}

<footer>
  <div class="wrap">
    <p>DJ Mantra by Marko Boško, Mantra Productions · <a href="https://djmantra.pages.dev">djmantra.pages.dev</a> · <a href="https://github.com/markoboskoauroville/djmantra_app">source code</a></p>
    <p>Built on Mixxx 2.5.6 (GPL v2 or later). “Hercules” and “DJControl” are trademarks of Guillemot Corporation; DJ Mantra is not affiliated with Hercules. As an Amazon Associate I earn from qualifying purchases; the TIDAL link may also earn a commission.</p>
    <p>Songs in the lesson player: Secret ID · Moojo &amp; Da Capo · 2023 · Calamar Records; I Follow Rivers · Sean Finn, Play!On · 2025 · ℗ Mukoko Groove, original by Lykke Li. Played for teaching.</p>
  </div>
</footer>

<script>
// The lesson's tabs (instrumental | vocal) and the section colours, as on djmantra.pages.dev.
(() => {{
  const G = document.getElementById("lesson");
  const COL = {{INTRO:"--s-intro", BREAKDOWN:"--s-break", "BUILD-UP":"--s-build", DROP:"--s-drop", MAIN:"--s-main", OUTRO:"--s-outro",
    VERSE:"--s-verse", "PRE-CHORUS":"--s-pre", CHORUS:"--s-chorus", BRIDGE:"--s-bridge"}};
  const css = v => getComputedStyle(document.documentElement).getPropertyValue(v).trim() || "#888";
  G.querySelectorAll(".sc").forEach(c => c.style.setProperty("--c", css(COL[c.dataset.s] || "--s-amb")));
  const tabs = [...G.querySelectorAll(".tabs button")];
  tabs.forEach(b => b.addEventListener("click", () => {{
    tabs.forEach(x => x.setAttribute("aria-selected", x === b ? "true" : "false"));
    G.querySelectorAll(".part").forEach(p => p.hidden = p.id !== b.dataset.part);
  }}));
}})();
{PLAYER_JS}
</script>
<script src="https://speechifykeys.pages.dev/speechify-keys.js" defer></script>
</body>
</html>
"""

(HERE / "public" / "index.html").write_text(PAGE)
print("wrote", HERE / "public" / "index.html", len(PAGE), "bytes")
