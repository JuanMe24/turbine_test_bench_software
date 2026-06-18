import serial
import time
import csv

# =====================================
# SERIAL CONFIGURATION
# =====================================

ser = serial.Serial(
    port='COM6',
    baudrate=115200,
    timeout=0.1
)

print("Connected to COM6")

# =====================================
# CAN INITIALIZATION
# =====================================

print("Initializing CAN adapter...")

ser.write(b'C\r')      # Close CAN channel
time.sleep(0.2)

ser.write(b'S4\r')     # 125 kbps (purple LED according to your tests)
time.sleep(0.2)

ser.write(b'O\r')      # Open CAN channel
time.sleep(0.2)

ser.reset_input_buffer()

print("CAN adapter initialized")
print("Listening to encoder stream...")

# =====================================
# ENCODER PARAMETERS
# =====================================

COUNTS_PER_REV = 1024

# =====================================
# CSV FILE
# =====================================

csv_file = open(
    "encoder_data_stream.csv",
    mode="w",
    newline=""
)

csv_writer = csv.writer(csv_file)

csv_writer.writerow([
    "time_s",
    "counts",
    "angle_deg"
])

print("CSV logging started")

# =====================================
# TIME REFERENCE
# =====================================

t0 = time.perf_counter()

# =====================================
# STATISTICS
# =====================================

packet_count = 0
last_report = time.perf_counter()

# =====================================
# MAIN LOOP
# =====================================

try:

    while True:

        packet = ser.read(9)

        if len(packet) != 9:
            continue

        # Verify packet header
        if (
            packet[0] != 0x01 or
            packet[1] != 0x03 or
            packet[2] != 0x04
        ):
            continue

        # Extract encoder counts
        counts = int.from_bytes(
            packet[3:7],
            byteorder='big'
        )

        if not (0 <= counts <= 1023):
            continue

        # Convert to angle
        angle_deg = (
            counts * 360.0
            / COUNTS_PER_REV
        )

        # Timestamp
        t = time.perf_counter() - t0

        # Save sample
        csv_writer.writerow([
            t,
            counts,
            angle_deg
        ])

        packet_count += 1

        # Flush every 100 samples
        if packet_count % 100 == 0:
            csv_file.flush()

        # Report once per second
        now = time.perf_counter()

        if now - last_report >= 1.0:

            rate = packet_count / t

            print(
                f"Packets: {packet_count} | "
                f"Rate: {rate:.1f} Hz"
            )

            print(
                f"t={t:8.3f}s | "
                f"Counts={counts:4d} | "
                f"Angle={angle_deg:8.2f} deg"
            )

            last_report = now

except KeyboardInterrupt:

    print("\nStopping acquisition...")

finally:

    csv_file.flush()
    csv_file.close()

    ser.close()

    print("CSV saved.")
    print("Serial port closed.")