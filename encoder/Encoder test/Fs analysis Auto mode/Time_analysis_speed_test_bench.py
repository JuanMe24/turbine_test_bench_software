import pandas as pd
import numpy as np

# =====================================
# LOAD CSV
# =====================================

df = pd.read_csv("timing_test.csv")

t = df["time_s"].to_numpy()

# =====================================
# DT
# =====================================

dt = np.diff(t)

# =====================================
# STATISTICS
# =====================================

print("\n========== RESULTS ==========\n")

print(
    f"Samples      : {len(t)}"
)

print(
    f"Duration     : {t[-1]-t[0]:.3f} s"
)

print(
    f"Mean Ts      : {np.mean(dt)*1000:.3f} ms"
)

print(
    f"Median Ts    : {np.median(dt)*1000:.3f} ms"
)

print(
    f"Std Ts       : {np.std(dt)*1000:.3f} ms"
)

print(
    f"Min Ts       : {np.min(dt)*1000:.3f} ms"
)

print(
    f"Max Ts       : {np.max(dt)*1000:.3f} ms"
)

print(
    f"Mean Fs      : {1/np.mean(dt):.3f} Hz"
)

print(
    f"Median Fs    : {1/np.median(dt):.3f} Hz"
)