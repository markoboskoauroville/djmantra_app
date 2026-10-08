#!/usr/bin/env python3
"""Record what a MIDI controller sends, and send it test messages (LEDs).

Built for capturing the Hercules DJControl Mix Ultra, step by step, from a
Claude Code session: every command is non-interactive and appends to one
JSON log, so the session can ask the user to press a control, run `listen`,
and move on.

    pip install mido python-rtmidi

    midi_capture.py ports
    midi_capture.py listen --label "Deck A PLAY" --seconds 6
    midi_capture.py send 91 07 7F            # e.g. light the PLAY LED
    midi_capture.py sweep 91 00 7F --delay 0.4   # every note 00..7F on 0x91
    midi_capture.py summary                  # Markdown table of the log

The port is picked by --port (substring, case-insensitive); default "hercules".
The log defaults to testing/results/mix-ultra-capture.json.
"""

import argparse
import json
import sys
import time
from datetime import datetime
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
DEFAULT_LOG = REPO / "testing" / "results" / "mix-ultra-capture.json"


def need_mido():
    try:
        import mido  # noqa: F401
    except ImportError:
        sys.exit("mido is missing: pip install mido python-rtmidi")
    import mido

    return mido


def pick(names, wanted):
    hits = [n for n in names if wanted.lower() in n.lower()]
    if not hits:
        sys.exit(f"No MIDI port matches '{wanted}'. Ports: {names}")
    return hits[0]


def hexbytes(msg):
    return " ".join(f"{b:02X}" for b in msg.bytes())


def load_log(path):
    if path.exists():
        return json.loads(path.read_text(encoding="utf-8"))
    return {"device": None, "captures": [], "sent": []}


def save_log(path, log):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(log, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def describe(messages):
    """Group raw messages into controls: (status, number) -> values seen."""
    controls = {}
    for m in messages:
        b = m["bytes"]
        if len(b) < 3 or b[0] >= 0xF0:
            continue
        key = f"{b[0]:02X} {b[1]:02X}"
        c = controls.setdefault(key, {"count": 0, "values": set()})
        c["count"] += 1
        c["values"].add(b[2])
    out = []
    for key, c in controls.items():
        values = sorted(c["values"])
        status = int(key.split()[0], 16)
        number = int(key.split()[1], 16)
        kind = {0x80: "note-off", 0x90: "note", 0xB0: "cc"}.get(status & 0xF0, "other")
        # 14-bit pair: CC n (MSB) together with CC n+0x20 (LSB)
        lsb = f"{status:02X} {number + 0x20:02X}"
        out.append({
            "control": key,
            "kind": kind,
            "channel": (status & 0x0F) + 1,
            "count": c["count"],
            "min": values[0],
            "max": values[-1],
            "distinct": len(values),
            "values": values if len(values) <= 8 else values[:4] + ["..."] + values[-4:],
            "has_lsb_pair": kind == "cc" and number < 0x20 and lsb in controls,
        })
    return sorted(out, key=lambda x: x["control"])


def cmd_ports(args):
    mido = need_mido()
    print("inputs: ", mido.get_input_names())
    print("outputs:", mido.get_output_names())


def cmd_listen(args):
    mido = need_mido()
    name = pick(mido.get_input_names(), args.port)
    messages = []
    start = time.monotonic()
    with mido.open_input(name) as port:
        print(f"Listening on {name} for {args.seconds}s: {args.label}", flush=True)
        while time.monotonic() - start < args.seconds:
            for msg in port.iter_pending():
                messages.append({
                    "t": round(time.monotonic() - start, 4),
                    "bytes": list(msg.bytes()),
                    "hex": hexbytes(msg),
                })
            time.sleep(0.001)
    log = load_log(args.log)
    log["device"] = name
    summary = describe(messages)
    log["captures"].append({
        "label": args.label,
        "at": datetime.now().isoformat(timespec="seconds"),
        "seconds": args.seconds,
        "summary": summary,
        "messages": messages[: args.keep],
        "total_messages": len(messages),
    })
    save_log(args.log, log)
    print(json.dumps({"label": args.label, "total": len(messages), "controls": summary},
                     indent=1, default=list))


def parse_hex(values):
    return [int(v, 16) for v in values]


def cmd_send(args):
    mido = need_mido()
    name = pick(mido.get_output_names(), args.port)
    data = parse_hex(args.bytes)
    with mido.open_output(name) as port:
        port.send(mido.Message.from_bytes(data))
    log = load_log(args.log)
    log["sent"].append({"hex": " ".join(args.bytes).upper(), "note": args.note,
                        "at": datetime.now().isoformat(timespec="seconds")})
    save_log(args.log, log)
    print("sent", " ".join(f"{b:02X}" for b in data), "to", name)


def cmd_sweep(args):
    """Send status/number/value for number = first..last, one by one, so the
    user can say which LED lights at which number."""
    mido = need_mido()
    name = pick(mido.get_output_names(), args.port)
    status, first, value = parse_hex([args.status, args.first, args.value])
    last = int(args.last, 16)
    with mido.open_output(name) as port:
        for number in range(first, last + 1):
            port.send(mido.Message.from_bytes([status, number, value]))
            print(f"{status:02X} {number:02X} {value:02X}", flush=True)
            time.sleep(args.delay)
            if not args.keep_on:
                port.send(mido.Message.from_bytes([status, number, 0]))


def cmd_summary(args):
    log = load_log(args.log)
    print(f"Device: {log.get('device')}\n")
    print("| Label | Control | Kind | Ch | Values | 14-bit |")
    print("|---|---|---|---|---|---|")
    for cap in log["captures"]:
        if not cap["summary"]:
            print(f"| {cap['label']} | (nothing) | | | | |")
        for c in cap["summary"]:
            vals = f"{c['min']:02X}..{c['max']:02X} ({c['distinct']})"
            print(f"| {cap['label']} | `{c['control']}` | {c['kind']} | {c['channel']} "
                  f"| {vals} | {'yes' if c['has_lsb_pair'] else ''} |")
    if log["sent"]:
        print("\n| Sent | Observed |")
        print("|---|---|")
        for s in log["sent"]:
            print(f"| `{s['hex']}` | {s.get('note') or ''} |")


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--port", default="hercules", help="port name substring")
    p.add_argument("--log", type=Path, default=DEFAULT_LOG)
    sub = p.add_subparsers(dest="cmd", required=True)
    sub.add_parser("ports").set_defaults(fn=cmd_ports)
    s = sub.add_parser("listen")
    s.add_argument("--label", required=True)
    s.add_argument("--seconds", type=float, default=6)
    s.add_argument("--keep", type=int, default=400, help="raw messages kept in the log")
    s.set_defaults(fn=cmd_listen)
    s = sub.add_parser("send")
    s.add_argument("bytes", nargs="+", help="hex bytes, e.g. 91 07 7F")
    s.add_argument("--note", default="", help="what the user saw (recorded in the log)")
    s.set_defaults(fn=cmd_send)
    s = sub.add_parser("sweep")
    s.add_argument("status")
    s.add_argument("first")
    s.add_argument("last")
    s.add_argument("--value", default="7F")
    s.add_argument("--delay", type=float, default=0.5)
    s.add_argument("--keep-on", action="store_true")
    s.set_defaults(fn=cmd_sweep)
    sub.add_parser("summary").set_defaults(fn=cmd_summary)
    args = p.parse_args()
    args.fn(args)


if __name__ == "__main__":
    main()
