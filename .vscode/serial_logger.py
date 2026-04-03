#!/usr/bin/env python3
"""
serial_logger.py — Serial monitor with timestamped logging.

Opens a serial port, optionally resets the board via DTR, then streams
serial output to both stdout and a log file in extras/logs/.

Log file name format (compatible with read_serial_log.py):
    <port>_<YYYY_MM_DD.HH.MM.SS.mmm>.txt
    e.g. devttyUSB0_2026_03_13.12.33.38.000.txt

Each log line is prefixed with a timestamp:
    HH:MM:SS:mmm <content>
    e.g. 12:33:38:152 Hello, world!

Board/port/baud defaults are read from .vscode/arduino.json when present.

Usage:
    serial_logger.py [--port PORT] [--baud BAUD] [--reset] [--log-dir DIR]

Options:
    --port      Serial port (e.g. /dev/ttyUSB0). Defaults to arduino.json 'port'.
    --baud      Baud rate. Defaults to arduino.json 'baudRate' or 115200.
    --reset     Toggle DTR to reset the board before monitoring.
    --log-dir   Directory for log files. Defaults to <workspace>/extras/logs/.
    --timeout   Stop automatically after this many seconds (0 = run until Ctrl-C).
    --stop-on   Stop when a line matches this Python regex (e.g. "PASS|FAIL|DONE").

Exit codes:
    0  Clean exit (Ctrl-C) or sentinel matched
    1  Serial port error
    2  Missing or invalid arguments
    3  Could not create log directory
    4  Timeout reached without finding the sentinel pattern
"""
import argparse
import json
import os
import signal
import sys
import time
from datetime import datetime
from pathlib import Path

try:
    import serial
except ImportError:
    print("Error: pyserial is required.  Install with:  pip install pyserial",
          file=sys.stderr)
    sys.exit(2)


# ---------------------------------------------------------------------------
# Configuration helpers
# ---------------------------------------------------------------------------

def _workspace() -> Path:
    ws = os.environ.get('WORKSPACE_FOLDER')
    if ws:
        return Path(ws).resolve()
    # Script lives in <workspace>/.vscode/
    return Path(__file__).resolve().parent.parent


def _read_arduino_json() -> dict:
    path = _workspace() / '.vscode' / 'arduino.json'
    try:
        return json.loads(path.read_text())
    except Exception:
        return {}


# ---------------------------------------------------------------------------
# Log path helpers
# ---------------------------------------------------------------------------

def _port_stem(port: str) -> str:
    """'/dev/ttyUSB0' -> 'devttyUSB0'"""
    return port.lstrip('/').replace('/', '')


def _log_filename(port: str) -> str:
    now = datetime.now()
    ts = now.strftime('%Y_%m_%d.%H.%M.%S') + f'.{now.microsecond // 1000:03d}'
    return f'{_port_stem(port)}_{ts}.txt'


def _ts_prefix() -> str:
    """Return 'HH:MM:SS:mmm ' for the current moment."""
    now = datetime.now()
    ms = now.microsecond // 1000
    return f'{now.hour:02d}:{now.minute:02d}:{now.second:02d}:{ms:03d} '


# ---------------------------------------------------------------------------
# Main monitor loop
# ---------------------------------------------------------------------------

def run(port: str, baud: int, reset: bool, log_dir: Path,
        timeout: float = 0.0, stop_on: str | None = None) -> int:
    # Ensure log directory exists
    try:
        log_dir.mkdir(parents=True, exist_ok=True)
    except OSError as e:
        print(f'Error: cannot create log directory {log_dir}: {e}', file=sys.stderr)
        return 3

    log_path = log_dir / _log_filename(port)

    print(f'Opening {port} @ {baud} baud')
    print(f'Log file: {log_path}')
    print('Press Ctrl-C to stop.\n')

    try:
        ser = serial.Serial(port, baud, timeout=0.1)
    except serial.SerialException as e:
        print(f'Error: cannot open {port}: {e}', file=sys.stderr)
        return 1

    if reset:
        print('Resetting board via DTR ...')
        ser.setDTR(False)
        time.sleep(0.1)
        ser.setDTR(True)
        time.sleep(0.1)
        print('Reset complete.\n')

    # Handle Ctrl-C gracefully via flag
    _stop = [False]

    def _sigint(sig, frame):
        _stop[0] = True

    signal.signal(signal.SIGINT, _sigint)

    import re as _re
    stop_re = _re.compile(stop_on) if stop_on else None
    start_time = time.monotonic()
    exit_code = 0

    buf = b''
    with open(log_path, 'w', encoding='utf-8', errors='replace') as logf:
        while not _stop[0]:
            # ── Timeout check ──────────────────────────────────────────────
            if timeout > 0 and (time.monotonic() - start_time) >= timeout:
                suffix = f' (sentinel "{stop_on}" not found)' if stop_on else ''
                marker = _ts_prefix() + f'[LOGGER] Stopped: timeout after {timeout:.0f}s{suffix}'
                print(marker)
                logf.write(marker + '\n')
                exit_code = 4
                break

            try:
                chunk = ser.read(256)
            except serial.SerialException as e:
                print(f'\nSerial error: {e}', file=sys.stderr)
                exit_code = 1
                break

            if not chunk:
                continue

            buf += chunk
            sentinel_matched = False
            while b'\n' in buf:
                line_bytes, buf = buf.split(b'\n', 1)
                line = line_bytes.decode('utf-8', errors='replace').rstrip('\r')
                entry = _ts_prefix() + line
                print(entry)
                logf.write(entry + '\n')
                logf.flush()
                # ── Sentinel check ─────────────────────────────────────────
                if stop_re and stop_re.search(line):
                    marker = _ts_prefix() + f'[LOGGER] Stopped: sentinel matched "{stop_on}"'
                    print(marker)
                    logf.write(marker + '\n')
                    logf.flush()
                    sentinel_matched = True
                    break
            if sentinel_matched:
                break

    # Flush any incomplete line remaining in the buffer
    if buf:
        line = buf.decode('utf-8', errors='replace').rstrip('\r\n')
        if line:
            entry = _ts_prefix() + line
            print(entry)
            with open(log_path, 'a', encoding='utf-8', errors='replace') as logf:
                logf.write(entry + '\n')

    try:
        ser.close()
    except Exception:
        pass

    print(f'\nLog saved to: {log_path}')
    return exit_code


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    cfg = _read_arduino_json()
    ws = _workspace()

    parser = argparse.ArgumentParser(
        description='Serial monitor with timestamped logging.')
    parser.add_argument(
        '--port', default=cfg.get('port') or '',
        help='Serial port (e.g. /dev/ttyUSB0)')
    parser.add_argument(
        '--baud', type=int, default=cfg.get('baudRate', 115200),
        help='Baud rate (default: from arduino.json or 115200)')
    parser.add_argument(
        '--reset', action='store_true',
        help='Toggle DTR to reset the board before monitoring')
    parser.add_argument(
        '--log-dir', default=str(ws / 'extras' / 'logs'),
        help='Log output directory (default: <workspace>/extras/logs/)')
    parser.add_argument(
        '--timeout', type=float, default=0.0, metavar='SECONDS',
        help='Stop after this many seconds (0 = run until Ctrl-C)')
    parser.add_argument(
        '--stop-on', default='', metavar='REGEX',
        help='Stop when a line matches this Python regex (e.g. "PASS|FAIL|DONE")')
    args = parser.parse_args()

    if not args.port:
        # Auto-detect: pick most-recently-modified tty
        import glob as _glob
        candidates = sorted(
            _glob.glob('/dev/ttyACM*') + _glob.glob('/dev/ttyUSB*'),
            key=os.path.getmtime, reverse=True)
        if not candidates:
            print('Error: no serial port specified and none auto-detected.',
                  file=sys.stderr)
            print('Use --port /dev/ttyUSB0 or set "port" in .vscode/arduino.json',
                  file=sys.stderr)
            sys.exit(2)
        args.port = candidates[0]
        print(f'Auto-detected port: {args.port}')

    sys.exit(run(args.port, args.baud, args.reset, Path(args.log_dir),
               args.timeout, args.stop_on or None))


if __name__ == '__main__':
    main()
