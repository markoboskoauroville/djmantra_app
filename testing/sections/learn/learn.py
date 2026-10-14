import os
import numpy as np, sys
sys.path.insert(0,"/home/user/djmantra_app/testing/sections"); import sections as sec
S="os.environ.get('BENCH', '.')"
data=np.load(S+"/ds.npy",allow_pickle=True).item()
def feats(v):
    b3,q=v["b3"],v["q"]; nb=len(b3); L=np.log1p(b3)
    f=sec.zscore(np.c_[3*L,q]); f=f/(np.linalg.norm(f,axis=1,keepdims=True)+1e-9); ssm=f@f.T
    nov={h:sec.checker_novelty(ssm,h) for h in (1,2,4,8)}
    def ch(w):
        o=np.zeros((nb,3))
        for i in range(w,nb-w+1):
            o[i]=np.abs(np.log((b3[i:i+w].mean(0)+4)/(b3[i-w:i].mean(0)+4)))
        return o
    c1,c2,c4=ch(1),ch(2),ch(4)
    k=np.arange(nb)
    X=np.c_[nov[1],nov[2],nov[4],nov[8],c1,c2,c4,c4.max(1),
            (k%4==0),(k%8==0),(k%16==0),(k<2),(k>nb-3), L.sum(1)/3]
    return X
songs=sorted(data); rng=np.random.default_rng(0); rng.shuffle(songs)
train,test=songs[::2],songs[1::2]
Xs={s:feats(data[s]) for s in songs}
def stack(ss): return np.vstack([Xs[s] for s in ss]), np.concatenate([data[s]["lab"] for s in ss])
Xtr,ytr=stack(train); mu,sd=Xtr.mean(0),Xtr.std(0)+1e-9
def norm(X): return (X-mu)/sd
# logistic regression, plain gradient descent, class-balanced
X=norm(Xtr); w=np.zeros(X.shape[1]); b=0.0; pw=(1-ytr.mean())/ytr.mean()
for it in range(3000):
    z=X@w+b; p=1/(1+np.exp(-z)); g=(p-ytr)*np.where(ytr==1,pw,1)
    w-=0.05*(X.T@g/len(ytr)+1e-3*w); b-=0.05*g.mean()
def decode(pr,thr,gap):
    out=[]
    for i in np.argsort(-pr):
        if pr[i]<thr: break
        if all(abs(i-j)>=gap for j in out): out.append(i)
    return sorted(out)
def evaluate(ss,thr,gap,tol=0):
    H=T=P=0
    for s in ss:
        pr=1/(1+np.exp(-(norm(Xs[s])@w+b))); f=decode(pr,thr,gap); t=np.where(data[s]["lab"]==1)[0]
        H+=sum(any(abs(a-c)<=tol for a in f) for c in t); T+=len(t); P+=len(f)
    pr_=H/max(P,1); rc=H/max(T,1); return 2*pr_*rc/max(pr_+rc,1e-9),pr_,rc
best=max(((evaluate(train,t,g),t,g) for t in np.arange(0.3,0.95,0.05) for g in (2,3,4)),key=lambda x:x[0][0])
(f,p,r),thr,gap=best
print(f"chosen on train: thr {thr:.2f} gap {gap}")
for nm,ss in (("train",train),("TEST",test)):
    for tol in (0,1):
        f,p,r=evaluate(ss,thr,gap,tol); print(f"{nm:5s} exact bar{' ±1 bar' if tol else '       '}: F1 {f:.2f} precision {p:.2f} recall {r:.2f}")
names=["nov1","nov2","nov4","nov8","c1L","c1M","c1H","c2L","c2M","c2H","c4L","c4M","c4H","c4max","mod4","mod8","mod16","start","end","energy"]
print("weights:",{n:round(float(x),2) for n,x in zip(names,w)})
np.save(S+"/model.npy",dict(w=w,b=b,mu=mu,sd=sd,thr=thr,gap=gap),allow_pickle=True)
