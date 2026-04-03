#!/usr/bin/env python3
"""
serial_reset_monitor.py — Toggle DTR to reset an Arduino/ESP32 board.

Monitoring and log-file creation are handled by serial_logger.py.
This script only performs the hardware reset.

Usage:
  serial_reset_monitor.py <port> [baud]

The baud rate is required to open the serial port; it does not affect the
DTR-toggle reset but must match what the device expects (default: 115200).
"""
import argparse
import sys
import time

try:
    import serial
except ImportError:
    print("Error: pyserial is required.  Install with:  pip install pyserial")
    sys.exit(1)


def reset_board(port: str, baud: int) -> int:
    print(f"Opening {port} @ {baud} baud ...")
    try:
        s = serial.Serial(port, baud, timeout=1)
    except Exception as e:
        print(f"Error: cannot open {port}: {e}")
        return 1
    try:
        print("Toggling DTR to reset the board ...")
        s.setDTR(False)
        time.sleep(0.1)
        s.setDTR(True)
        time.sleep(0.1)
        print("Reset complete.")
    finally:
        try:
            s.close()
        except Exception:
            pass
    return 0


def main():
    p = argparse.ArgumentParser(
        description='Toggle DTR to reset an Arduino/ESP32 board.')
    p.add_argument('port', help='Serial port, e.g. /dev/ttyACM0')
    p.add_argument('baud', nargs='?', type=int, default=115200,
                   help='Baud rate used to open the port (default: 115200)')
    args = p.parse_args()
    sys.exit(reset_board(args.port, args.baud))


if __name__ == '__main__':
    main()
