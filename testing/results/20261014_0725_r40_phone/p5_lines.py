import sys, subprocess, numpy as np
vid, y0, y1 = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
W=1080
raw = subprocess.check_output(['ffmpeg','-loglevel','quiet','-i',vid,'-vf',f'crop={W}:{y1-y0}:0:{y0}','-fps_mode','passthrough','-f','rawvideo','-pix_fmt','gray','-'])
h=y1-y0; n=len(raw)//(W*h)
fr=np.frombuffer(raw,np.uint8)[:n*W*h].reshape(n,h,W).astype(float).mean(axis=1)
def lines(p):
    xs=[x for x in range(40,W-120) if p[x]>p.mean()+40 and p[x]>=p[x-1] and p[x]>=p[x+1] and abs(x-W//2)>8]
    # merge neighbours
    out=[]
    for x in xs:
        if out and x-out[-1]<4: continue
        out.append(x)
    return out
prev=None; shifts=[]; widths=[]
for i in range(n):
    L=lines(fr[i])
    # line width: count of columns above threshold around each line
    if prev is not None and L and prev:
        d=[min((a-b for b in prev), key=abs) for a in L]
        shifts.append(int(np.median(d)))
    prev=L
print(n,'frames, lines in frame0:',lines(fr[0]))
print('shifts:',' '.join(map(str,shifts)))
s=np.array(shifts); print('mean %.2f min %d max %d' % (s.mean(), s.min(), s.max()))
