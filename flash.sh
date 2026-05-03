#!/bin/bash
set -e

UF2="${1:-$(dirname "$0")/tmp/build/icosaedro.uf2}"

if [ ! -f "$UF2" ]; then
    echo "UF2 non trovato: $UF2"
    exit 1
fi

PORT=$(ls /dev/cu.usbmodem* 2>/dev/null | head -1)
if [ -z "$PORT" ]; then
    echo "Nessuna porta usbmodem trovata"
    exit 1
fi

echo "Reset 1200 baud su $PORT..."
python3 - "$PORT" <<'PYEOF'
import sys, os, termios, time
port = sys.argv[1]
try:
    fd = os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    attrs = termios.tcgetattr(fd)
    attrs[4] = termios.B1200
    attrs[5] = termios.B1200
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    time.sleep(0.25)
    os.close(fd)
except Exception as e:
    print(f"serial: {e}")
PYEOF

echo "Attendo RPI-RP2 (max 15s)..."
for i in $(seq 1 30); do
    if [ -d /Volumes/RPI-RP2 ]; then
        break
    fi
    sleep 0.5
done

if [ ! -d /Volumes/RPI-RP2 ]; then
    echo "Timeout: RPI-RP2 non trovato."
    exit 1
fi

echo "Copio $(basename "$UF2") -> /Volumes/RPI-RP2/"
cp "$UF2" /Volumes/RPI-RP2/
echo "Flash completato."
