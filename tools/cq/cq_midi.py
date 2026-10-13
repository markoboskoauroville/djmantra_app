"""Allen & Heath CQ MIDI over TCP (official CQ MIDI Protocol V1.2 issue 4: port 51325, MIDI channel 1,
one TCP connection at a time). A test tool for testing/CQ12T_FX.md.

  python3 cq_midi.py <ip> get <msb> <lsb>          the current value of a mute / level / pan parameter (hex)
  python3 cq_midi.py <ip> set <msb> <lsb> <vc> <vf>  absolute value (level / pan; mute: vc 00, vf 01 on / 00 off)
  python3 cq_midi.py <ip> tap <softkey 1-3> <bpm> [n]  n soft key presses (Note On 0x30+k-1, then Note Off) a beat apart
  python3 cq_midi.py <ip> listen <seconds>          print what the CQ sends (it sends NRPN when its controls move)
Parameters (hex): Ip1 mute 00 00, FX1..4 mute 00 51..54, Main LR mute 00 44; Ip1 -> Main LR level 40 00;
Ip1 -> FX1..4 send 4C 14..17; Main LR level 4F 00; FX1..4 unit input 4F 0D..10. Level 0 dB = 62 00, -inf = 00 00."""
import socket, sys, time

PORT = 51325

def nrpn(msb, lsb):
    return bytes([0xB0, 0x63, msb, 0xB0, 0x62, lsb])

def connect(ip):
    s = socket.create_connection((ip, PORT), timeout=3)
    s.settimeout(0.3)
    return s

def drain(s, secs=0.5):
    out, end = b'', time.time() + secs
    while time.time() < end:
        try:
            b = s.recv(4096)
            if not b:
                break
            out += b
        except socket.timeout:
            pass
    return out

def get(s, msb, lsb):
    drain(s, 0.1)
    s.sendall(nrpn(msb, lsb) + bytes([0xB0, 0x60, 0x7F]))
    return drain(s, 0.6)

def setv(s, msb, lsb, vc, vf):
    s.sendall(nrpn(msb, lsb) + bytes([0xB0, 0x06, vc, 0xB0, 0x26, vf]))

def tap(s, key, bpm, n=4):
    note, gap, t0 = 0x30 + key - 1, 60.0 / bpm, time.perf_counter()
    for i in range(n):
        while time.perf_counter() < t0 + i * gap:
            pass
        s.sendall(bytes([0x90, note, 0x7F]))
        time.sleep(0.03)
        s.sendall(bytes([0x80, note, 0x00]))

def hx(b):
    return ' '.join(f'{x:02X}' for x in b)

if __name__ == '__main__':
    ip, cmd, a = sys.argv[1], sys.argv[2], sys.argv[3:]
    s = connect(ip)
    try:
        if cmd == 'get':
            print(hx(get(s, int(a[0], 16), int(a[1], 16))))
        elif cmd == 'set':
            setv(s, *[int(v, 16) for v in a[:4]]); print('sent', hx(drain(s, 0.4)))
        elif cmd == 'tap':
            tap(s, int(a[0]), float(a[1]), int(a[2]) if len(a) > 2 else 4); print('tapped')
        elif cmd == 'listen':
            print(hx(drain(s, float(a[0]))))
    finally:
        s.close()
