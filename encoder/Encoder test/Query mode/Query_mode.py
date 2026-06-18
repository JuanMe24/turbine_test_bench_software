import serial
import time
import csv

# =====================================
# SERIAL CONFIGURATION
# =====================================

ser = serial.Serial(
    port='COM6',
    baudrate=115200,
    timeout=0.05
)

print("Connected")

# =====================================
# MODBUS QUERY
# =====================================

query = bytes.fromhex(
    "01 03 00 00 00 01 84 0A"
)

# =====================================
# ENCODER PARAMETERS
# =====================================

COUNTS_PER_REV = 1024

# =====================================
# CSV FILE
# =====================================

csv_file = open(
    "encoder_query_mode.csv",
    mode="w",
    newline=""
)

csv_writer = csv.writer(csv_file)

csv_writer.writerow([
    "time_s",
    "counts",
    "angle_deg"
])

# =====================================
# ACQUISITION PARAMETERS
# =====================================

FS_TARGET = 100.0
TS_TARGET = 1.0 / FS_TARGET

print(f"Target Fs = {FS_TARGET:.1f} Hz")
print(f"Target Ts = {TS_TARGET*1000:.1f} ms")

# =====================================
# TIME REFERENCE
# =====================================

t0 = time.perf_counter()

packet_count = 0
last_report = time.perf_counter()

# =====================================
# MAIN LOOP
# =====================================

try:

    next_sample_time = time.perf_counter()

    while True:

        # Wait until next sample instant
        while time.perf_counter() < next_sample_time:
            pass

        next_sample_time += TS_TARGET

        # ---------------------------------
        # SEND QUERY
        # ---------------------------------

        ser.write(query)

        # ---------------------------------
        # READ RESPONSE
        # ---------------------------------

        response = ser.read(9)

        if len(response) != 9:
            continue

        # Verify header
        if (
            response[0] != 0x01 or
            response[1] != 0x03 or
            response[2] != 0x04
        ):
            continue

        # Extract position
        counts = int.from_bytes(
            response[3:7],
            byteorder='big'
        )

        if not (0 <= counts <= 1023):
            continue

        # Convert to degrees
        angle_deg = (
            counts * 360.0
            / COUNTS_PER_REV
        )

        # Timestamp
        t = time.perf_counter() - t0

        # Save CSV
        csv_writer.writerow([
            t,
            counts,
            angle_deg
        ])

        packet_count += 1

        # Flush every 100 samples
        if packet_count % 100 == 0:
            csv_file.flush()

        # Report every second
        now = time.perf_counter()

        if now - last_report >= 1.0:

            fs_measured = packet_count / t

            print(
                f"Packets={packet_count} | "
                f"Fs={fs_measured:.2f} Hz | "
                f"Counts={counts} | "
                f"Angle={angle_deg:.2f} deg"
            )

            last_report = now

except KeyboardInterrupt:

    print("\nStopping acquisition...")

finally:

    total_time = time.perf_counter() - t0

    if total_time > 0:

        fs_final = packet_count / total_time

        print("\n========== FINAL RESULTS ==========\n")

        print(f"Samples  : {packet_count}")
        print(f"Duration : {total_time:.3f} s")
        print(f"Fs       : {fs_final:.3f} Hz")

    csv_file.flush()
    csv_file.close()

    ser.close()

    print("\nCSV saved.")
    print("Serial port closed.")