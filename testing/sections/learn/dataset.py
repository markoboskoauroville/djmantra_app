import os
# per bar (rekordbox's grid) features from the app-like bands; label = a rekordbox phrase starts on this bar
import json, glob, os, sys, numpy as np, urllib.parse, xml.etree.ElementTree as ET
sys.path.insert(0,"/home/user/djmantra_app/testing/sections"); import sections as sec
S="os.environ.get('BENCH', '.')"
rb=json.load(open("/home/user/djmantra_app/testing/sections/rekordbox.json"))
grids={}
for t in ET.parse(sys.argv[1]).getroot().iter("TRACK"):
    loc=t.get("Location")
    if not loc: continue
    name=os.path.basename(urllib.parse.unquote(urllib.parse.urlparse(loc).path))
    mk=[(float(m.get("Inizio")),float(m.get("Bpm")),int(m.get("Battito"))) for m in t.findall("TEMPO")]
    if mk: grids[name]=(mk,float(t.get("TotalTime")))
def beats_from(mk,total):
    out=[]
    for i,(s,bpm,bat) in enumerate(mk):
        end=mk[i+1][0] if i+1<len(mk) else total
        k=0
        while s+k*60/bpm < end-1e-6:
            out.append((s+k*60/bpm, (bat-1+k)%4)); k+=1
    return out
FPS=sec.FPS; data={}
for p in sorted(glob.glob(S+"/bench/npz/*.npz")):
    name=os.path.basename(p)[:-4]+".mp3"
    if name not in rb or name not in grids: continue
    d=np.load(p); lmh=d["lmh"].astype(float)
    bt=beats_from(*grids[name]); times=np.array([b[0] for b in bt]); pos=np.array([b[1] for b in bt])
    down=times[pos==0]
    if len(down)<16: continue
    starts=down[:-1]*FPS; ends=down[1:]*FPS
    b3=sec.per_span(lmh,starts,ends); b3=230*b3/(b3.max(axis=0)+1e-9)
    q=[]
    for s0,e0 in zip(starts,ends):
        ed=np.linspace(s0,e0,5); q.append(np.concatenate([sec.per_span(lmh,ed[i:i+1],ed[i+1:i+2])[0] for i in range(4)]))
    q=np.log1p(np.array(q))
    # phrase starts -> bar index (nearest downbeat)
    lab=np.zeros(len(starts),int)
    for ph in rb[name]["phrases"][1:]:
        i=int(np.argmin(np.abs(down[:-1]-ph["s"])))
        if abs(down[i]-ph["s"])<0.3: lab[i]=1
    data[name]=dict(b3=b3,q=q,lab=lab,down=down[:-1],low=lmh[:,0],starts=starts,ends=ends)
np.save(S+"/ds.npy",data,allow_pickle=True)
n=sum(len(v["lab"]) for v in data.values()); pz=sum(v["lab"].sum() for v in data.values())
print(len(data),"songs,",n,"bars,",pz,"phrase starts on a bar")
