"""Section detection test bench for DJ Mantra (tester's tool, not app code).

    python3 -I sections.py OUTDIR song.f32 [song.f32 ...]

Input: mono float32 22050 Hz (ffmpeg -ac 1 -ar 22050 -f f32le). Per song:
  - a constant-tempo beat grid, downbeats and the 8-bar phrase phase
  - A = the app's detector today (sectiondetector.cpp: 3 waveform bands per
    bar, 4 bars before vs 4 after, |log ratio| > 0.5, keep the stronger of
    neighbours 4 bars apart)
  - B = a reference: bar-synchronous self-similarity (timbre, harmony, bands)
    with checkerboard novelty, plus the kick going away / coming back, peaks
    snapped to the phrase grid, labelled like Rekordbox's phrase analysis
  - OUTDIR/<song>.png (the picture) and OUTDIR/<song>.json (the numbers)
"""
import json
import os
import sys

import numpy as np

SR = 22050
NFFT = 2048
HOP = 512
FPS = SR / HOP


# ---------- signal ----------

def stft_mag(x):
    win = np.hanning(NFFT).astype(np.float32)
    n = 1 + (len(x) - NFFT) // HOP
    out = np.empty((n, NFFT // 2 + 1), np.float32)
    for s in range(0, n, 2048):
        idx = np.arange(s, min(n, s + 2048))[:, None] * HOP + np.arange(NFFT)[None, :]
        out[s:s + len(idx)] = np.abs(np.fft.rfft(x[idx] * win, axis=1))
    return out


def mel_bank(nmels=40, fmin=30.0, fmax=11000.0):
    hz = np.fft.rfftfreq(NFFT, 1 / SR)
    m = lambda f: 2595 * np.log10(1 + f / 700)
    im = lambda v: 700 * (10 ** (v / 2595) - 1)
    pts = im(np.linspace(m(fmin), m(fmax), nmels + 2))
    fb = np.zeros((nmels, len(hz)), np.float32)
    for i in range(nmels):
        lo, c, hi = pts[i], pts[i + 1], pts[i + 2]
        fb[i] = np.clip(np.minimum((hz - lo) / (c - lo), (hi - hz) / (hi - c)), 0, None)
    return fb


def chroma_bank():
    hz = np.fft.rfftfreq(NFFT, 1 / SR)
    fb = np.zeros((12, len(hz)), np.float32)
    ok = (hz > 55) & (hz < 5000)
    pc = np.round(12 * np.log2(hz[ok] / 440.0)).astype(int) % 12
    fb[pc, np.where(ok)[0]] = 1.0
    return fb


def band(mag, lo, hi):
    hz = np.fft.rfftfreq(NFFT, 1 / SR)
    sel = (hz >= lo) & (hz < hi)
    return np.sqrt((mag[:, sel] ** 2).sum(axis=1))


# ---------- beat grid ----------

def beat_grid(onset, n_frames):
    """Constant tempo (electronic music): every BPM 80-160 in 0.05 steps with
    the best of 24 phases, then +-0.1 BPM in 0.005 steps and 1/4-frame phase."""
    def fit(bpm, phases):
        period = 60 * FPS / bpm
        best = (-1, 0)
        for ph in phases(period):
            t = np.round(np.arange(ph, n_frames - 1, period)).astype(int)
            v = onset[t].mean()
            if v > best[0]:
                best = (v, ph)
        return best
    coarse = lambda period: np.linspace(0, period, 24, endpoint=False)
    scores = [(fit(b, coarse)[0], b) for b in np.arange(80, 160, 0.05)]
    bpm0 = max(scores)[1]
    best = (-1, bpm0, 0)
    for bpm in np.arange(bpm0 - 0.1, bpm0 + 0.1, 0.005):
        v, ph = fit(bpm, lambda period: np.arange(0, period, 0.25))
        if v > best[0]:
            best = (v, bpm, ph)
    _, bpm, ph = best
    period = 60 * FPS / bpm
    return round(float(bpm), 3), np.arange(ph % period, n_frames - 1, period)


def per_span(feat, starts, ends):
    """Mean of frame features between start and end frames (per bar)."""
    out = []
    for a, b in zip(starts, ends):
        a, b = int(round(a)), max(int(round(a)) + 1, int(round(b)))
        out.append(feat[a:b].mean(axis=0))
    return np.array(out)


# ---------- A: the app today ----------

def detector_app(bands3, phase_bar0):
    """Port of djmantra::sectionBars (threshold 0.5, floor 4, 4-bar phrases).
    bands3: per bar {low, mid, high} scaled like the app's waveform (0..255)."""
    k, floor, thr = 4, 4.0, 0.5
    bars = len(bands3)
    cands = []
    first = phase_bar0 % k
    for bar in range(first, bars, k):
        if bar < k or bar + k > bars:
            continue
        score = 0.0
        for b in range(3):
            before = bands3[bar - k:bar, b].mean()
            after = bands3[bar:bar + k, b].mean()
            score = max(score, abs(np.log((after + floor) / (before + floor))))
        if score > thr:
            cands.append((bar, score))
    out = []
    for i, (bar, s) in enumerate(cands):
        wp = i > 0 and cands[i - 1][0] == bar - k and cands[i - 1][1] > s
        wn = i + 1 < len(cands) and cands[i + 1][0] == bar + k and cands[i + 1][1] >= s
        if not wp and not wn:
            out.append(bar)
    return out


# ---------- C: the app's next detector (v2, builder 12.10.2026) ----------
# Only what the app has at run time: the waveform's low / mid / high per point
# (frames here) and the beat grid. Ported to sectiondetector.cpp once it scores
# well against B on the songs.

def detector_v2(lmh, beats, n_frames):
    """lmh: per frame low / mid / high (app scale); beats: beat frames.
    Returns (bar starts in frames, major section start bars, phrase phase)."""
    lmh = np.asarray(lmh, float)
    # 1. downbeat: the beat phase whose bar-to-bar band change is largest
    best = (-1, 0)
    for p in range(4):
        bs = beats[p::4]
        if len(bs) < 12:
            continue
        f = per_span(lmh, bs[:-1], bs[1:])
        ch = np.abs(np.diff(np.log1p(f), axis=0)).sum()
        if ch > best[0]:
            best = (ch, p)
    bars = beats[best[1]::4]
    starts, ends = bars[:-1], bars[1:]
    nb = len(starts)
    if nb < 16:
        return starts, [], 0
    b3 = per_span(lmh, starts, ends)
    b3 = 230 * b3 / (b3.max(axis=0) + 1e-9)
    # quarters of each bar (the rhythm pattern), log
    q = []
    for s0, e0 in zip(starts, ends):
        edges = np.linspace(s0, e0, 5)
        q.append(np.concatenate([per_span(lmh, edges[i:i + 1], edges[i + 1:i + 2])[0] for i in range(4)]))
    q = np.log1p(np.array(q))
    # 2. kick per bar: the low band's peak on each beat over its level between beats
    low = lmh[:, 0]
    pulse = []
    for s0, e0 in zip(starts, ends):
        edges = np.linspace(s0, e0, 5)
        v = []
        for i in range(4):
            a, b = int(edges[i]), max(int(edges[i]) + 1, int(edges[i + 1]))
            seg = low[a:b]
            head = seg[:max(1, len(seg) // 4)]
            v.append((head.max() - np.median(seg)) / (seg.max() + 1e-9) if len(seg) else 0)
        pulse.append(np.mean(v))
    pulse = np.array(pulse)
    k = pulse * b3[:, 0]
    kick = np.clip(k / (np.percentile(k, 90) + 1e-9), 0, 1.5)
    on = np.zeros(nb, bool)
    state = kick[0] > 0.5
    for i, v in enumerate(kick):
        if state and v < 0.45:  # (tuned: 0.35 / 0.6 before)
            state = False
        elif not state and v > 0.7:
            state = True
        on[i] = state
    i = 0
    while i < nb:  # a 1-bar state is a fill
        j = i
        while j < nb and on[j] == on[i]:
            j += 1
        if j - i < 2 and i > 0:
            on[i:j] = on[i - 1]
        i = j
    kch = np.r_[0, (on[1:] != on[:-1]).astype(float)]
    # band change over 4 bars (the app's old score)
    bch = np.zeros(nb)
    for i in range(4, nb - 4):
        bch[i] = max(abs(np.log((b3[i:i + 4, c].mean() + 4) / (b3[i - 4:i, c].mean() + 4))) for c in range(3))
    # 3. phrase "1": the 8-bar offset where the kick and the bands change most
    ch = np.r_[0, np.abs(np.diff(np.log1p(b3), axis=0)).sum(axis=1)]
    # (tuned on Marko's 150 songs: the kick in the vote cost 7 % exact; bands only)
    phase = int(np.argmax([ch[o::8].mean() for o in range(8)]))
    # 4. novelty: self-similarity of the bars (bands + their rhythm), checkerboard 4 and 8
    f = zscore(np.c_[3 * np.log1p(b3), q])
    f = f / (np.linalg.norm(f, axis=1, keepdims=True) + 1e-9)
    ssm = f @ f.T
    nov = 0.5 * checker_novelty(ssm, 4) + 0.5 * checker_novelty(ssm, 8)
    bc = np.clip(bch - 0.3, 0, None)
    score = nov + 0.6 * kch + 0.8 * bc
    rel = (np.arange(nb) - phase) % 8
    score = score * np.where(rel == 0, 1.0, np.where(rel == 4, 0.75, 0.35))
    thr = np.median(score) + 1.5 * np.median(np.abs(score - np.median(score))) + 0.15
    peaks = []
    for i in np.argsort(-score):
        if score[i] < thr or i < 4 or i > nb - 4:
            continue
        if all(abs(i - p) >= 8 for p in peaks):
            peaks.append(int(i))
    for i in np.where(kch > 0)[0]:
        if rel[i] in (0, 4) and 4 <= i <= nb - 1 and all(abs(i - p) >= 4 for p in peaks):
            peaks.append(int(i))
    major = sorted(p for p in peaks if kch[max(0, p - 1):p + 2].any() or nov[p] >= 0.5 or bc[p] >= 0.35)
    return starts, major, phase


def compare(found, truth, tol=0):
    """exact, off (1-4 bars), missed, false: found vs truth (bars)."""
    exact = sum(any(abs(f - t) <= tol for f in found) for t in truth)
    near = sum(not any(abs(f - t) <= tol for f in found) and any(abs(f - t) <= 4 for f in found) for t in truth)
    missed = len(truth) - exact - near
    false = sum(not any(abs(f - t) <= 4 for t in truth) for f in found)
    return {"exact": exact, "off_1_4": near, "missed": missed, "false": false}


# ---------- B: reference ----------

def zscore(m):
    return (m - m.mean(axis=0)) / (m.std(axis=0) + 1e-6)


def checker_novelty(ssm, half):
    n = len(ssm)
    g = np.exp(-0.5 * (np.linspace(-1.5, 1.5, 2 * half) ** 2))
    gk = np.outer(g, g)
    sign = np.ones((2 * half, 2 * half))
    sign[:half, half:] = -1
    sign[half:, :half] = -1
    k = gk * sign
    pad = np.pad(ssm, half, mode="edge")
    nov = np.zeros(n)
    for i in range(n):
        nov[i] = (pad[i:i + 2 * half, i:i + 2 * half] * k).sum()
    nov = np.clip(nov, 0, None)
    return nov / (nov.max() + 1e-9)


def detector_reference(barfeat, kick, phrase_phase, n_bars, bands3=None):
    """barfeat: per bar feature matrix; kick: per bar kick presence 0..1.
    Returns (section start bars, novelty, kick states)."""
    f = zscore(barfeat)
    f = f / (np.linalg.norm(f, axis=1, keepdims=True) + 1e-9)
    ssm = f @ f.T
    nov = 0.5 * checker_novelty(ssm, 4) + 0.5 * checker_novelty(ssm, 8)
    # kick on/off with hysteresis, smoothed over 2 bars
    on = np.zeros(n_bars, bool)
    state = kick[0] > 0.5
    for i, v in enumerate(kick):
        if state and v < 0.35:
            state = False
        elif not state and v > 0.6:
            state = True
        on[i] = state
    # a fill (1 bar without kick at a phrase end) is not a change: states shorter than 2 bars merge
    i = 0
    while i < n_bars:
        j = i
        while j < n_bars and on[j] == on[i]:
            j += 1
        if j - i < 2 and i > 0:
            on[i:j] = on[i - 1]
        i = j
    kick_change = np.zeros(n_bars)
    kick_change[1:] = (on[1:] != on[:-1]).astype(float)
    # the score at each bar; phrase lines get a bonus (8 bars strong, 4 weak)
    # the app's own idea, kept: a band (bass coming in, highs going) changing by
    # |log ratio| over 4 bars before / after
    band_change = np.zeros(n_bars)
    if bands3 is not None:
        for i in range(4, n_bars - 4):
            band_change[i] = max(abs(np.log((bands3[i:i + 4, k].mean() + 4) /
                                             (bands3[i - 4:i, k].mean() + 4))) for k in range(3))
        band_change = np.clip(band_change - 0.3, 0, None)
    score = nov + 0.6 * kick_change + 0.8 * band_change
    rel = (np.arange(n_bars) - phrase_phase) % 8
    grid = np.where(rel == 0, 1.0, np.where(rel == 4, 0.75, 0.35))
    score = score * grid
    thr = np.median(score) + 1.5 * np.median(np.abs(score - np.median(score))) + 0.15
    peaks = []
    order = np.argsort(-score)
    for i in order:
        if score[i] < thr or i < 4 or i > n_bars - 4:
            continue
        if all(abs(i - p) >= 8 for p in peaks):
            peaks.append(int(i))
    # a kick change exactly on an 8-bar line is always a section (drops, breaks)
    for i in np.where(kick_change > 0)[0]:
        if rel[i] in (0, 4) and 4 <= i <= n_bars - 1 and all(abs(i - p) >= 4 for p in peaks):
            peaks.append(int(i))
    peaks = sorted(peaks)
    major = [p for p in peaks if kick_change[max(0, p - 1):p + 2].any() or nov[p] >= 0.5 or band_change[p] >= 0.35]
    return peaks, major, nov, on, ssm


def label(sections, on, energy, n_bars):
    """Rekordbox-like names by the kick: kick on and loud = Chorus, kick on and
    quieter = Up; no kick before the first Chorus = Intro, after the last = Outro,
    in between = Down."""
    bounds = [0] + sections + [n_bars]
    segs = []
    for j in range(len(bounds) - 1):
        a, b = bounds[j], bounds[j + 1]
        segs.append((on[a:b].mean() > 0.5, energy[a:b].mean()))
    kick_segs = [j for j, (k, _) in enumerate(segs) if k]
    first, last = (kick_segs[0], kick_segs[-1]) if kick_segs else (len(segs), -1)
    loud = np.median([e for k, e in segs if k]) if kick_segs else 0
    names = []
    for j, (k, e) in enumerate(segs):
        if k:
            names.append("Chorus" if e >= loud - 0.15 else "Up")
        elif j < first:
            names.append("Intro")
        elif j > last:
            names.append("Outro")
        else:
            names.append("Down")
    return bounds, names


# ---------- per song ----------

def analyse(path, outdir):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    name = os.path.splitext(os.path.basename(path))[0]
    x = np.fromfile(path, np.float32)
    mag = stft_mag(x)
    n = len(mag)
    mel = np.log1p(100 * (mag @ mel_bank().T))
    chroma = mag @ chroma_bank().T
    chroma = chroma / (chroma.sum(axis=1, keepdims=True) + 1e-9)
    onset = np.clip(np.diff(mel, axis=0, prepend=mel[:1]), 0, None).sum(axis=1)
    onset = onset / (onset.max() + 1e-9)
    bpm, beats = beat_grid(onset, n)

    # app-like bands (Mixxx waveform: low < 600 Hz, mid 600-4000, high > 4000)
    low, mid, high = band(mag, 20, 600), band(mag, 600, 4000), band(mag, 4000, 11025)
    sub = band(mag, 35, 110)  # kick + sub bass
    # downbeat: the bar phase whose bar-to-bar change is largest
    best = (-1, 0)
    for p in range(4):
        bs = beats[p::4]
        f = per_span(np.c_[low, mid, high], bs[:-1], bs[1:])
        ch = np.abs(np.diff(np.log1p(f), axis=0)).sum()
        if ch > best[0]:
            best = (ch, p)
    bar_starts = beats[best[1]::4]
    starts, ends = bar_starts[:-1], bar_starts[1:]
    nb = len(starts)
    b3 = per_span(np.c_[low, mid, high], starts, ends)
    b3 = 230 * b3 / b3.max(axis=0)  # the waveform's 0..255 scale (per band peak)
    # phrase phase: the 8-bar offset where the bands change most
    ch = np.r_[0, np.abs(np.diff(np.log1p(b3), axis=0)).sum(axis=1)]
    phase = None  # set below from the kick

    mel_b = per_span(mel, starts, ends)
    chr_b = per_span(chroma, starts, ends)
    sub_b = per_span(sub[:, None], starts, ends)[:, 0]
    # kick presence: the sub band's beat-rate pulse (onsets on the beats)
    pulse = []
    for s, e in zip(starts, ends):
        s, e = int(s), int(e)
        seg = sub[s:e]
        pulse.append((seg.max() - np.median(seg)) / (seg.max() + 1e-9) if len(seg) else 0)
    pulse = np.array(pulse)
    kick = np.clip((pulse * sub_b) / (np.percentile(pulse * sub_b, 90) + 1e-9), 0, 1.5)
    kon = kick > 0.5
    kch = np.r_[0, (kon[1:] != kon[:-1]).astype(float)]
    # a 1-bar dip that comes back is a fill: its END marks a phrase start, not its beginning
    vote = [2.0 * kch[q::8].sum() + ch[q::8].mean() for q in range(8)]
    phase = int(np.argmax(vote))
    a = detector_app(b3, 0)  # the app counts its 4-bar phrases from the song's "1" (bar 0)
    feat = np.c_[mel_b, 2 * chr_b, 3 * np.log1p(b3)]
    b, major, nov, on, ssm = detector_reference(feat, kick, phase, nb, b3)
    energy = np.log1p(b3).sum(axis=1)
    bounds, names = label(b, on, energy, nb)

    if os.environ.get("SECTIONS_DUMP"):  # the inputs, to tune the detector without decoding again
        np.savez_compressed(os.path.join(os.environ["SECTIONS_DUMP"], name + ".npz"),
                            lmh=np.c_[low, mid, high].astype(np.float32), beats=beats, n=n,
                            ref_starts=starts, ref_major=np.array(major, int))
    # C: the app's next detector, from the app-like bands only, on its own bars
    c_starts, c_major, c_phase = detector_v2(np.c_[low, mid, high], beats, n)
    # its bars in the reference's bar numbers (both from the same beat grid)
    c_major_ref = [int(np.argmin(np.abs(starts - c_starts[i]))) for i in c_major]

    t = lambda bar: float(starts[min(bar, nb - 1)] / FPS)
    result = {
        "song": name, "bpm": bpm, "bars": nb, "downbeat_phase": best[1], "phrase_phase": phase,
        "app_sections_s": [round(t(i), 1) for i in a],
        "app_sections_bar": [int(i) for i in a],
        "ref_sections_s": [round(t(i), 1) for i in b],
        "ref_sections_bar": b,
        "ref_major_bar": major,
        "ref_major_s": [round(t(i), 1) for i in major],
        "ref_labels": [{"bar": bounds[j], "s": round(t(bounds[j]), 1), "name": names[j]}
                       for j in range(len(names))],
        "v2_major_bar": c_major_ref,
        "v2_major_s": [round(t(i), 1) for i in c_major_ref],
        "v2_vs_ref_major": compare(c_major_ref, major),
        "app_vs_ref_major": compare(list(a), major),
        "app_on_8bar_grid": sum((i - phase) % 8 == 0 for i in a),
        "ref_on_8bar_grid": sum((i - phase) % 8 == 0 for i in b),
    }
    with open(os.path.join(outdir, name + ".json"), "w") as fh:
        json.dump(result, fh, indent=1)

    # picture
    fig = plt.figure(figsize=(12, 7.2), dpi=100)
    gs = fig.add_gridspec(5, 2, width_ratios=[4, 1], height_ratios=[2.2, 1, 1, 1, 0.6])
    ax0 = fig.add_subplot(gs[0, 0])
    ax0.imshow(mel_b.T, aspect="auto", origin="lower", cmap="magma",
               extent=[0, nb, 0, mel_b.shape[1]])
    ax0.set_ylabel("timbre\n(40 mel)")
    ax0.set_title(f"{name}   {bpm} BPM, {nb} bars   red = app today, cyan = reference (solid major, dotted minor), green dashed = v2")
    ax1 = fig.add_subplot(gs[1, 0], sharex=ax0)
    for i, (c, l) in enumerate(zip("rgb", ("low", "mid", "high"))):
        ax1.plot(np.arange(nb) + 0.5, b3[:, i], c, lw=1, label=l)
    ax1.set_ylabel("bands\n(app)")
    ax1.legend(loc="upper right", fontsize=7, ncol=3)
    ax2 = fig.add_subplot(gs[2, 0], sharex=ax0)
    ax2.fill_between(np.arange(nb) + 0.5, 0, np.clip(kick, 0, 1.2), color="0.6", step="mid")
    ax2.plot(np.arange(nb) + 0.5, on * 1.1, "k", lw=1, drawstyle="steps-mid")
    ax2.set_ylabel("kick")
    ax3 = fig.add_subplot(gs[3, 0], sharex=ax0)
    ax3.plot(np.arange(nb) + 0.5, nov, "c", lw=1.2)
    ax3.set_ylabel("novelty")
    for ax in (ax0, ax1, ax2, ax3):
        for g in range(phase, nb, 8):
            ax.axvline(g, color="0.85", lw=0.5, zorder=0)
        for i in a:
            ax.axvline(i, color="red", lw=1.6, alpha=0.8)
        for i in b:
            ax.axvline(i, color="cyan", lw=2.2 if i in major else 1.0, ls="-" if i in major else ":")
        for i in c_major_ref:
            ax.axvline(i + 0.15, color="lime", lw=1.4, ls="--")
    ax4 = fig.add_subplot(gs[4, 0], sharex=ax0)
    colours = {"Intro": "#8fb3ff", "Up": "#ffd27f", "Down": "#9be59b", "Chorus": "#ff8f8f",
               "Outro": "#c9a8ff"}
    for j, nm in enumerate(names):
        ax4.axvspan(bounds[j], bounds[j + 1], color=colours[nm])
        ax4.text((bounds[j] + bounds[j + 1]) / 2, 0.5, nm, ha="center", va="center", fontsize=8)
    ax4.set_yticks([])
    ax4.set_xlabel("bar (grey lines = 8-bar phrases)")
    ax4.set_xlim(0, nb)
    axs = fig.add_subplot(gs[:, 1])
    axs.imshow(ssm, cmap="gray", origin="lower")
    for i in b:
        axs.axhline(i, color="cyan", lw=0.6)
        axs.axvline(i, color="cyan", lw=0.6)
    axs.set_title("bar self-similarity", fontsize=9)
    axs.set_xticks([])
    axs.set_yticks([])
    fig.tight_layout()
    fig.savefig(os.path.join(outdir, name + ".png"))
    plt.close(fig)
    return result


if __name__ == "__main__":
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    tot = {"app": {}, "v2": {}}
    for p in sys.argv[2:]:
        r = analyse(p, out)
        for key, field in (("app", "app_vs_ref_major"), ("v2", "v2_vs_ref_major")):
            for k2, v in r[field].items():
                tot[key][k2] = tot[key].get(k2, 0) + v
        print(f"   v2 vs ref major: {r['v2_vs_ref_major']}   app: {r['app_vs_ref_major']}")
        print(f"{r['song'][:28]:28s} {r['bpm']:6.2f} BPM {r['bars']:4d} bars  "
              f"app {len(r['app_sections_bar']):2d} ({r['app_on_8bar_grid']} on 8-grid)  "
              f"ref {len(r['ref_sections_bar']):2d} ({r['ref_on_8bar_grid']} on 8-grid)")
    print("TOTAL vs reference major sections: app", tot["app"], " v2", tot["v2"])
