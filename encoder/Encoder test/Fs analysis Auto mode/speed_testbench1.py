import serial
import time
import csv

# =====================================
# SERIAL
# =====================================

ser = serial.Serial(
    port='COM6',
    baudrate=115200,
    timeout=1
)

# =====================================
# CSV
# =====================================

csv_file = open(
    "timing_test.csv",
    "w",
    newline=""
)

writer = csv.writer(csv_file)

writer.writerow([
    "sample",
    "time_s"
])

# =====================================
# START
# =====================================

print("Recording...")

t0 = time.perf_counter()

sample = 0

try:

    while True:

        packet = ser.read(9)

        if len(packet) != 9:
            continue

        if (
            packet[0] != 0x01 or
            packet[1] != 0x03 or
            packet[2] != 0x04
        ):
            continue

        t = time.perf_counter() - t0

        sample += 1

        writer.writerow([
            sample,
            t
        ])

        if sample % 100 == 0:

            csv_file.flush()

            print(
                f"Samples = {sample}"
            )

except KeyboardInterrupt:

    print("\nStopping...")

finally:

    csv_file.close()
    ser.close()