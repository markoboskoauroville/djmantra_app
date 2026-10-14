import subprocess, time, json, sys
S = sys.argv[1]; O = S + '/fx2'
def osa(s): return subprocess.run(['osascript', '-e', s], capture_output=True, text=True).stdout.strip()
def fx(i): osa(f'tell application "System Events" to tell process "djay Pro" to click menu item {i} of menu 1 of menu item "FX" of menu 1 of menu bar item "Deck 1" of menu bar 1')
def d1(n): osa(f'tell application "System Events" to tell process "djay Pro" to click menu item "{n}" of menu 1 of menu bar item "Deck 1" of menu bar 1')
def front(): osa('tell application "System Events" to set frontmost of process "djay Pro" to true')
def drag(x0, x1, y=528):
    front(); subprocess.run(['cliclick', f'dd:{x0},{y}', 'w:80', f'm:{(x0+x1)//2},{y}', 'w:80', f'm:{x1},{y}', 'w:80', f'du:{x1},{y}'])
def click(x, y=528): front(); subprocess.run(['cliclick', f'c:{x},{y}'])
def shot(n): subprocess.run(['screencapture', '-x', '-D', '2', f'{O}/{n}.png'])
def level():
    subprocess.run(['ffmpeg','-v','quiet','-y','-f','avfoundation','-i',':1','-t','1',f'{O}/_lv.wav'])
    r = subprocess.run(['ffmpeg','-i',f'{O}/_lv.wav','-af','volumedetect','-f','null','-'],capture_output=True,text=True).stderr
    return [l.split(':')[1].strip() for l in r.splitlines() if 'mean_volume' in l][0]
allog = {}
def take(name, steps, start=58.0):
    d1('Go to Beginning'); time.sleep(0.4); d1('Play / Pause'); tp = time.time()
    time.sleep(start - 1.5); lv0 = level()
    rec = subprocess.Popen(['ffmpeg','-v','quiet','-y','-f','avfoundation','-i',':1','-ac','2','-ar','48000',f'{O}/{name}.wav'])
    t0 = time.time(); log = [(0.0, f'rec start, song at {t0-tp:.1f} s, level before {lv0}')]
    for label, act, dur in steps:
        if act: act()
        log.append((round(time.time()-t0, 2), label)); time.sleep(dur)
    rec.terminate(); rec.wait(); lv1 = level(); shot(name + '_end')
    log.append((round(time.time()-t0, 2), f'rec end, level after {lv1}'))
    d1('Play / Pause'); time.sleep(0.8); log.append((None, f'after pause level {level()}'))
    allog[name] = log; print(name, log, flush=True)
S1 = {'left': 2622, 'mid': 2663, 'right': 2705}; S2 = {'left': 2840, 'mid': 2881, 'right': 2922}
cur = {'1': S1['mid'], '2': S2['right']}
def mv(slot, pos):
    sl = S1 if slot == '1' else S2
    def f(): drag(cur[slot], sl[pos]); cur[slot] = sl[pos]
    return f
def basic(slot_idx, slot):
    return [('dry', None, 8), ('ON', lambda: fx(slot_idx), 8), ('OFF', lambda: fx(slot_idx), 4)]
def sliders(slot_idx, slot):
    return [('dry, slider left', mv(slot, 'left'), 3), ('ON slider left', lambda: fx(slot_idx), 4),
            ('slider mid', mv(slot, 'mid'), 4), ('slider right', mv(slot, 'right'), 4), ('OFF', lambda: fx(slot_idx), 4)]
# slot 2 is Cooldown now; set slot 2 slider to mid first
mv('2', 'mid')()
take('flanger_dry8_on8_off4', basic(1, '1'))
take('flanger_slider_left_mid_right', sliders(1, '1'))
mv('1', 'mid')()
take('cooldown_dry8_on8_off4', basic(6, '2'))
take('cooldown_slider_left_mid_right', sliders(6, '2'))
for _ in range(3): fx(7); time.sleep(0.4)   # Cooldown -> Echo Out -> Flanger -> Underwater
shot('slot2_underwater'); mv('2', 'mid')()
take('underwater_dry8_on8_off4', basic(6, '2'))
take('underwater_slider_left_mid_right', sliders(6, '2'))
click(3060); time.sleep(0.6); shot('echo_set_1')
take('echoout_1beat_dry4_on12_off4', [('dry', None, 4), ('ON', lambda: fx(11), 12), ('OFF', lambda: fx(11), 4)])
click(3132); time.sleep(0.6); shot('echo_set_2')
take('echoout_2beats_dry4_on12_off4', [('dry', None, 4), ('ON', lambda: fx(11), 12), ('OFF', lambda: fx(11), 4)])
json.dump(allog, open(f'{O}/log.json', 'w'), indent=1)
