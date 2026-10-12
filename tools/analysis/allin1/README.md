# All-In-One (allin1) in the cloud container

All-In-One (mir-aidj/all-in-one) gives beats, downbeats, section boundaries and labels.
We run it here only as a reference for the section detector, the same way we use rekordbox.

Setup (Python 3.13, CPU):

    pip install torch cython mido "setuptools<81"
    pip install --no-build-isolation madmom==0.16.1
    # patch madmom: collections -> collections.abc; np.float/np.int/np.bool/np.complex -> float/int/bool/complex
    pip install natten==0.21.7 demucs huggingface-hub hydra-core omegaconf
    pip install --no-deps allin1==1.1.0
    cp natten_compat.py <site-packages>/
    sed -i 's/^from natten.functional import/from natten_compat import/' <site-packages>/allin1/models/dinat.py

`natten_compat.py` is a pure-torch version of the old NATTEN functions
(natten1dqkrpb, natten1dav, natten2dqkrpb, natten2dav), which newer NATTEN no longer has
and the old NATTEN cannot be built here.

Weights: allin1 downloads `taejunkim/allinone` from huggingface.co, and Demucs downloads
`htdemucs` (955717e8) from dl.fbaipublicfiles.com. Both hosts must be allowed in the
environment's Network access (Allowed domains): huggingface.co, cdn-lfs.huggingface.co,
cas-bridge.xethub.hf.co, dl.fbaipublicfiles.com.
