import serial
import time
import csv
import matplotlib.pyplot as plt
import numpy as np
import statistics

# ==========================
# Configuration
# ==========================
SERIAL_PORT = "/dev/ttyACM0"   # Windows: "COM3"
BAUD_RATE = 115200
LOG_DURATION_SEC = 10         # Total logging time
TARGET_SAMPLES = 2000

OUTPUT_CSV = "hc_sr04_log_100.csv"

true_dist = int(input("Enter the physical distance in cm: "))

# ==========================
# Serial Initialization
# ==========================

ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
time.sleep(2)  # Allow Arduino reset

print("Serial connection established.")

# ==========================
# Data Storage
# ==========================

timestamps = []
valid_samples = []
distances = []

# start_time = time.time()

# ==========================
# CSV Logging
# ==========================

# =============================================================================
# first_timestamp = None
# =============================================================================
sample_count = 0

with open(OUTPUT_CSV, "w", newline="") as csvfile:
     writer = csv.writer(csvfile)
     writer.writerow(["timestamp_ms ", "valid_samples ", "distance_cm", true_dist])

     while sample_count < TARGET_SAMPLES:
         line = ser.readline().decode("utf-8").strip()

         if not line:
             continue

         try:
             ts, valid, dist = line.split(", ")
             if dist == "nan" or dist == "NaN":
                 continue
             ts = int(ts)
             valid = int(valid)
             dist = float(dist)

# =============================================================================
#              if first_timestamp is None:
#                 first_timestamp = ts
# =============================================================================

             # Subtract the offset to start at 0
             #normalized_timestamp = ts - first_timestamp
             
             timestamps.append(ts)
             valid_samples.append(valid)
             distances.append(dist)

             writer.writerow([ts, dist, valid, true_dist])
             sample_count += 1

             if sample_count % 100 == 0:
                 print(f"{sample_count} samples collected")

         except ValueError:
             # Skip malformed lines
             continue

ser.close()
print("Logging complete. Data saved to:", OUTPUT_CSV)
print("Average distance = ", statistics.mean(distances))

# ==========================
# Plotting
# ==========================

# =============================================================================
# data = np.genfromtxt(
#     "hc_sr04_log.csv",
#     delimiter=",",
#     skip_header=1
# )
#
# timestamps = data[:, 0]
# distances = data[:, 1]
# valid_samples = data[:, 2]
# =============================================================================

plt.figure()
plt.plot(timestamps, distances)
plt.xlabel("Timestamp [ms]")
plt.ylabel("Distance [cm]")
plt.title(f"HC-SR04 Distance Measurements, {true_dist} cm")
plt.grid(True)
plt.show()

# =============================================================================
# plt.figure()
# plt.plot(timestamps, valid_samples)
# plt.xlabel("Timestamp [ms]")
# plt.ylabel("Valid Samples Used")
# plt.title("Measurement Quality Indicator")
# plt.grid(True)
# plt.show()
# =============================================================================

