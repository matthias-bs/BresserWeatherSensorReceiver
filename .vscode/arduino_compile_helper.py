#!/usr/bin/env python3
"""
arduino_compile_helper.py

Helper to determine `fqbn` and sketch path and invoke arduino-cli (or the repo wrapper).

Behavior:
- Reads `.vscode/sketch.json` for `fqbn` and `sketch` when present.
- If `--fqbn` is provided it overrides sketch.json.
- If `--active` is provided, it will search upwards for the nearest `.ino` file and use that sketch's folder.
- Falls back to `examples/BresserWeatherSensorMQTT` when no sketch can be deduced.
"""
import argparse
import json
import os
import subprocess
import sys
from pathlib import Path


def read_arduino_json(workspace):
    path = Path(workspace) / '.vscode' / 'sketch.json'
    if not path.exists():
        return {}
    try:
        return json.loads(path.read_text())
    except Exception:
        return {}


def find_nearest_ino(active_path: Path, workspace: Path):
    # Walk up from active_path directory and look for any .ino file,
    # stopping at workspace boundary.
    p = active_path if active_path.is_dir() else active_path.parent
    while True:
        inos = list(p.glob('*.ino'))
        if inos:
            return p
        if p == workspace or p.parent == p:
            break
        p = p.parent
    # No .ino found upward — return None so callers can try other strategies
    return None


def find_wrapper(workspace: Path):
    w = workspace / '.vscode' / 'arduino-cli-wrapper.sh'
    if w.exists() and os.access(str(w), os.X_OK):
        return str(w)
    if w.exists():
        return str(w)
    return None


def write_arduino_json(workspace: Path, data: dict) -> None:
    """Persist updated fields back to .vscode/sketch.json."""
    path = Path(workspace) / '.vscode' / 'sketch.json'
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, indent=2) + '\n')


def detect_upload_ports() -> list[str]:
    """Return serial port candidates sorted by most-recently modified first."""
    import glob as _glob
    candidates = _glob.glob('/dev/ttyACM*') + _glob.glob('/dev/ttyUSB*')
    candidates.sort(key=lambda p: os.path.getmtime(p), reverse=True)
    return candidates


def try_upload(wrapper: str | None, fqbn: str | None, port: str, sketch: str,
               extra: list[str]) -> int:
    """Attempt a single upload and return the process exit code."""
    if wrapper:
        cmd = ['bash', wrapper, 'upload']
    else:
        cmd = ['arduino-cli', 'upload']
    if fqbn:
        cmd += ['--fqbn', fqbn]
    cmd += ['--port', port]
    cmd.append(sketch)
    if extra:
        cmd += [e for e in extra if e != '--']
    try:
        return subprocess.run(cmd).returncode
    except FileNotFoundError:
        print('arduino-cli or wrapper not found.', file=sys.stderr)
        return 2


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('action', choices=['compile', 'upload', 'monitor'])
    parser.add_argument('--fqbn', help='Override fqbn', default=None)
    parser.add_argument('--port', help='Serial port', default=None)
    parser.add_argument('--config', help='Extra config', default=None)
    parser.add_argument('--active', help='Active file path (to deduce sketch)', default=None)
    # parse_known_args() is used instead of parse_args() + REMAINDER because
    # argparse.REMAINDER eagerly captures all tokens after the first positional,
    # including named flags like --active, so they never reach their definition.
    args, extras = parser.parse_known_args()

    workspace = Path(os.environ.get('WORKSPACE_FOLDER', os.getcwd())).resolve()
    arduino_cfg = read_arduino_json(workspace)

    fqbn = args.fqbn or arduino_cfg.get('fqbn') or arduino_cfg.get('board')

    sketch_dir = None

    # Priority 1: --active → walk upward to nearest .ino
    if args.active:
        active = Path(args.active).resolve()
        sketch_dir = find_nearest_ino(active, workspace)

    # Priority 2: arduino.json 'sketch' entry
    if not sketch_dir:
        sketch_entry = arduino_cfg.get('sketch')
        if sketch_entry:
            cand = (workspace / sketch_entry).resolve()
            if cand.exists():
                sketch_dir = cand if cand.is_dir() else cand.parent

    # Priority 3: fallback to default example
    if not sketch_dir:
        sketch_dir = workspace / 'examples' / 'BresserWeatherSensorMQTT'

    sketch_path = str(sketch_dir)

    wrapper = find_wrapper(workspace)
    extras = [e for e in extras if e != '--']

    # ── Compile / Monitor: straightforward single-shot execution ─────────────
    if args.action in ('compile', 'monitor'):
        if wrapper:
            cmd = ['bash', wrapper, args.action]
        else:
            cmd = ['arduino-cli', args.action]
        if fqbn:
            cmd += ['--fqbn', fqbn]
        if args.port:
            cmd += ['--port', args.port]
        if args.config:
            cmd += ['--config', args.config]
        if args.action == 'compile':
            cmd.append(sketch_path)
        if extras:
            cmd += extras
        try:
            sys.exit(subprocess.run(cmd).returncode)
        except FileNotFoundError:
            print('arduino-cli or wrapper not found.', file=sys.stderr)
            sys.exit(2)

    # ── Upload: smart port detection with retry logic ──────────────────────
    # Port priority: CLI arg → arduino.json 'port' → auto-detect
    port_override = args.port or arduino_cfg.get('port')
    upload_mode   = arduino_cfg.get('uploadMode', 'auto')  # 'auto' or 'manual'

    def _try_all_ports(port_list: list[str]) -> int:
        for port in port_list:
            print(f'--- Trying port: {port} ---')
            rc = try_upload(wrapper, fqbn, port, sketch_path, extras)
            if rc == 0:
                # Remember the working port in arduino.json
                arduino_cfg['port'] = port
                write_arduino_json(workspace, arduino_cfg)
                print(f'Upload succeeded on {port}')
                return 0
            print(f'Upload failed on {port} (exit {rc})')
        return 1

    # Build candidate list
    if port_override:
        candidates = [port_override] + [p for p in detect_upload_ports() if p != port_override]
    else:
        candidates = detect_upload_ports()

    if not candidates:
        print('No serial ports detected (/dev/ttyACM* or /dev/ttyUSB*).', file=sys.stderr)
        if upload_mode != 'manual':
            print('Hint: if your board requires manual upload mode, set '
                  '"uploadMode": "manual" in .vscode/arduino.json', file=sys.stderr)
        sys.exit(1)

    # First attempt
    rc = _try_all_ports(candidates)
    if rc == 0:
        sys.exit(0)

    # If manual upload mode is known, prompt and retry once
    if upload_mode == 'manual':
        print()
        print('Board requires manual upload mode.')
        print('Please put the board into upload mode now (e.g. hold BOOT/double-press RESET),')
        input('then press ENTER to retry upload... ')
        # Re-detect ports: board may enumerate on a new tty after reset
        fresh = detect_upload_ports()
        rc = _try_all_ports(fresh)
        sys.exit(rc)
    else:
        print()
        print('All ports tried — upload did not succeed.')
        print('If your board needs manual upload mode (e.g. ESP32-S3 requires double-pressing RESET),')
        print('set "uploadMode": "manual" in .vscode/arduino.json and retry.')
        sys.exit(1)


if __name__ == '__main__':
    main()
