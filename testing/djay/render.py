"""Draws a song from its decoded djay JSON: the overview (low rate) and a zoomed strip (normal-rate colours)."""
import json, sys
import numpy as np
from PIL import Image

s = json.load(open(sys.argv[1])); out = sys.argv[2]
raw = bytes.fromhex(s['colors']['rgb565'])
v = np.frombuffer(raw, '<u2').astype(int)
rgb = np.stack([((v >> 11) & 31) * 255 // 31, ((v >> 5) & 63) * 255 // 63, (v & 31) * 255 // 31], 1).astype(np.uint8)
amp = np.array(s['lowRate']['max'], float) / 255
avg = np.array(s['lowRate']['avg'], float) / 255
W, H = 1600, 120
img = np.zeros((H * 3, W, 3), np.uint8)
# overview: height from lowRate max/avg, colour from the normal-rate colours resampled
for x in range(W):
    a = amp[int(x * len(amp) / W)]; m = avg[int(x * len(avg) / W)]
    c = rgb[int(x * len(rgb) / W)]
    h = int(a * H / 2); hm = int(m * H / 2)
    img[H // 2 - h:H // 2 + h, x] = c // 2
    img[H // 2 - hm:H // 2 + hm, x] = c
# colours alone, whole song
for x in range(W):
    img[H:2 * H, x] = rgb[int(x * len(rgb) / W)]
# zoom: 8 s from 60 s, one pixel per colour sample
start = int(60 * s['colors']['rate'])
seg = rgb[start:start + W]
img[2 * H:3 * H, :len(seg)] = seg[None, :, :]
Image.fromarray(img).save(out)
print(out, len(rgb), 'colour samples')
