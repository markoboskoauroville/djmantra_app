"""Reads djay Pro's per-song analysis (.djayMetadata, a binary plist) into plain arrays.

Where: ~/Library/Group Containers/VJXTL73S8G.com.algoriddim.userdata/Library/Application Support/Algoriddim/Metadata/<xx>/<md5>.djayMetadata
Floats inside the zlib blobs are big-endian float32; the colour arrays are little-endian RGB565.

  python3 djay_metadata.py <metadata dir> <out dir> [name filter file]
writes <out dir>/<song name>.json per song and <out dir>/index.json.
"""
import glob, json, os, plistlib, struct, sys, zlib


def f32be(blob):
    try:
        raw = zlib.decompress(blob)
    except zlib.error:  # an empty list is stored as a few bytes that are not a zlib stream
        return []
    return list(struct.unpack('>%df' % (len(raw) // 4), raw))


def rgb565(v):
    r, g, b = (v >> 11) & 31, (v >> 5) & 63, v & 31
    return (r * 255 // 31, g * 255 // 63, b * 255 // 31)


def read(path):
    d = plistlib.load(open(path, 'rb'))
    info, key, col = d.get('info', {}), d.get('keyInfo', {}), d.get('waveColorsInfo', {})
    wave, beat, struc = d.get('waveInfoCompact', {}), d.get('deepBeatTrackerInfo', {}), d.get('songStructureInfo', {})
    out = {'name': info.get('Name'), 'artist': info.get('Artist'), 'duration': info.get('Duration'),
           'file': os.path.basename(path)}
    if key:
        out['key'] = {k: key.get(k) for k in ('keyIndex', 'secondKeyIndex', 'keyConfidence', 'keyReferenceTuning')}
    if beat:
        out['beat'] = {k: beat.get(k) for k in ('bpm', 'analyzedBPM', 'straightBPM', 'bpmConfidence', 'straightGrid',
                                                 'firstDownBeatIndex', 'timeSignatureIndex', 'transientActivationFPS',
                                                 'deepBeatTrackerModelName')}
        for k, name in (('compressedBeats', 'beats'), ('compressedAlternateBeats', 'alternateBeats'),
                        ('compressedTransientPositions', 'transientPositions'),
                        ('compressedTransientEnergies', 'transientEnergies'),
                        ('compressedBPMChangeTimes', 'bpmChangeTimes'), ('compressedPrevalentBPMs', 'prevalentBPMs')):
            if k in beat:
                out['beat'][name] = [round(x, 5) for x in f32be(beat[k])]
    if struc:
        out['structure'] = {k: v for k, v in struc.items() if k != 'version'}
    if wave:
        out['lowRate'] = {'rate': wave.get('lowRateWaveFinalSampleRate'), 'peak': wave.get('lowRateWavePeak'),
                          'avg': list(zlib.decompress(wave['compressedLowRateWaveSamples'])),
                          'max': list(zlib.decompress(wave['compressedLowRateWaveSamplesMax']))}
    if col:
        # stored raw on a song loaded to a deck, zlib-compressed otherwise
        raw = col.get('normalRateWaveSamplesColors') or zlib.decompress(col['compressedNormalRateWaveSamplesColors'])
        out['colors'] = {'rate': col.get('normalRateSampleRate'), 'format': 'RGB565 little-endian, hex per sample',
                         'rgb565': raw.hex()}
        lr = zlib.decompress(col['compressedLowRateWaveColors'])
        out['lowRateColors'] = {'rate': col.get('lowRateSampleRate'), 'n': col.get('lowRateWaveTotalNumColors'),
                                'bytesPerItem': len(lr) // max(1, col.get('lowRateWaveTotalNumColors', 1)),
                                'hex': lr.hex()}
    return out


if __name__ == '__main__':
    src, dst = sys.argv[1:3]
    names = set(open(sys.argv[3]).read().split('\n')) if len(sys.argv) > 3 else None
    os.makedirs(dst, exist_ok=True)
    index = {}
    for p in glob.glob(os.path.join(src, '*', '*.djayMetadata')):
        try:
            s = read(p)
        except Exception as e:
            print('skip', p, e)
            continue
        if names is not None and s['name'] not in names:
            continue
        safe = (s['name'] or s['file']).replace('/', '_')
        json.dump(s, open(os.path.join(dst, safe + '.json'), 'w'), ensure_ascii=False)
        index[safe] = {'artist': s['artist'], 'bpm': s.get('beat', {}).get('bpm'),
                       'key': s.get('key', {}).get('keyIndex'), 'metadata': s['file']}
    json.dump(index, open(os.path.join(dst, 'index.json'), 'w'), indent=1, ensure_ascii=False)
    print(len(index), 'songs ->', dst)
