import os
import numpy as np, sys, glob, os
sys.path.insert(0,"/home/user/djmantra_app/testing/sections"); import sections as sec
S="os.environ.get('BENCH', '.')"
data=np.load(S+"/ds.npy",allow_pickle=True).item()
songs=sorted(data); rng=np.random.default_rng(0); rng.shuffle(songs); test=songs[1::2]
H=T=P=H1=0
for s in test:
    v=data[s]; d=np.load(S+"/bench/npz/"+s[:-4]+".npz")
    # rekordbox's beats (the downbeats + 3 between), as frames, given to v2
    st=v["starts"]; beats=np.concatenate([np.linspace(a,b,5)[:4] for a,b in zip(st,v["ends"])]+[[v["ends"][-1]]])
    cs,cm,ph=sec.detector_v2(d["lmh"],beats,int(d["n"]))
    f=[int(np.argmin(np.abs(st-cs[i]))) for i in cm]; t=np.where(v["lab"]==1)[0]
    H+=sum(any(a==c for a in f) for c in t); H1+=sum(any(abs(a-c)<=1 for a in f) for c in t); T+=len(t); P+=len(f)
for nm,h in (("exact bar",H),("±1 bar",H1)):
    p=h/P; r=h/T; print(f"v2 on rekordbox's grid, TEST {nm:9s}: F1 {2*p*r/(p+r):.2f} precision {p:.2f} recall {r:.2f}")
