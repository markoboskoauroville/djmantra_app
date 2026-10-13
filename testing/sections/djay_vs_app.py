"""Draws djay Pro's overview (from its decoded .djayMetadata JSON) with the app's section marks on top.
python3 djay_vs_app.py <djay.json> <out.png> <t1> <t2> ...   (section times in seconds)"""
import json, sys
import numpy as np
from PIL import Image, ImageDraw

s = json.load(open(sys.argv[1])); out = sys.argv[2]; marks = [float(t) for t in sys.argv[3:]]
dur = len(s['colors']['rgb565']) / 4 / s['colors']['rate']  # exact; 'duration' is whole seconds
v = np.frombuffer(bytes.fromhex(s['colors']['rgb565']), '<u2').astype(int)
rgb = np.stack([((v >> 11) & 31) * 255 // 31, ((v >> 5) & 63) * 255 // 63, (v & 31) * 255 // 31], 1).astype(np.uint8)
amp = np.array(s['lowRate']['max'], float) / 255; avg = np.array(s['lowRate']['avg'], float) / 255
W, H, T = 1600, 160, 22
img = np.full((H + T, W, 3), 20, np.uint8)
for x in range(W):
    a = amp[int(x * len(amp) / W)]; m = avg[int(x * len(avg) / W)]; c = rgb[int(x * len(rgb) / W)]
    h = int(a * H / 2); hm = int(m * H / 2)
    img[H // 2 - h:H // 2 + h, x] = c // 2
    img[H // 2 - hm:H // 2 + hm, x] = c
im = Image.fromarray(img); d = ImageDraw.Draw(im)
for t in range(0, int(dur) + 1, 30):  # time axis every 30 s
    x = int(t / dur * W); d.line([(x, H), (x, H + 6)], fill=(150, 150, 150)); d.text((x + 2, H + 6), f"{t // 60}:{t % 60:02d}", fill=(150, 150, 150))
for t in marks:  # the app's sections: white lines
    x = int(t / dur * W); d.line([(x, 0), (x, H)], fill=(255, 255, 255), width=2)
im.save(out); print(out, f"{dur:.0f} s", len(marks), "marks")
