"""Relay for testing the app's CQ MIDI from the phone while the CQ is cabled to the Mac (link-local).
  adb reverse tcp:51325 tcp:51325          (the phone's 127.0.0.1:51325 -> the Mac's 127.0.0.1:51325)
  python3 cq_relay.py 169.254.5.29         (the Mac's 127.0.0.1:51325 -> the CQ's 51325)
One client at a time, as the CQ allows one MIDI TCP connection; every message is printed in hex both ways."""
import socket, sys, threading, time

CQ, PORT = sys.argv[1], 51325

def pump(src, dst, tag):
    try:
        while True:
            b = src.recv(4096)
            if not b:
                break
            dst.sendall(b)
            print(time.strftime('%H:%M:%S'), tag, ' '.join(f'{x:02X}' for x in b), flush=True)
    except OSError:
        pass
    for s in (src, dst):
        try:
            s.shutdown(socket.SHUT_RDWR)
        except OSError:
            pass

srv = socket.socket(); srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
srv.bind(('127.0.0.1', PORT)); srv.listen(1)
print('relay 127.0.0.1:%d -> %s:%d' % (PORT, CQ, PORT), flush=True)
while True:
    c, _ = srv.accept()
    try:
        u = socket.create_connection((CQ, PORT), timeout=3); u.settimeout(None)
    except OSError as e:
        print('CQ not reachable:', e, flush=True); c.close(); continue
    print(time.strftime('%H:%M:%S'), 'client connected', flush=True)
    t = threading.Thread(target=pump, args=(u, c, 'CQ->app'), daemon=True); t.start()
    pump(c, u, 'app->CQ'); t.join()
    c.close(); u.close()
    print(time.strftime('%H:%M:%S'), 'client closed', flush=True)
