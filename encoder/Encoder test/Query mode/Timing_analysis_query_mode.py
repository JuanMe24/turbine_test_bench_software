import pandas as pd
import numpy as np

# =====================================
# READ CSV
# =====================================

df = pd.read_csv("encoder_query_mode.csv")

t = df["time_s"].to_numpy()

# =====================================
# DELTA T
# =====================================

dt = np.diff(t)

# =====================================
# RESULTS
# =====================================

print("\n========== DT DISTRIBUTION ==========\n")

print(f"Samples      : {len(dt)}")

print(f"Mean Ts      : {np.mean(dt)*1000:.6f} ms")
print(f"Median Ts    : {np.median(dt)*1000:.6f} ms")

print(f"Std Ts       : {np.std(dt)*1000:.6f} ms")

print(f"Min Ts       : {np.min(dt)*1000:.6f} ms")
print(f"Max Ts       : {np.max(dt)*1000:.6f} ms")

# =====================================
# PERCENTILES
# =====================================

print("\n========== PERCENTILES ==========\n")

for p in [1, 5, 25, 50, 75, 95, 99]:

    print(
        f"{p:2d}% : "
        f"{np.percentile(dt,p)*1000:.6f} ms"
    )

# =====================================
# 3-SIGMA TEST
# =====================================

mean_dt = np.mean(dt)
std_dt = np.std(dt)

inside = np.sum(
    np.abs(dt - mean_dt) <= 3*std_dt
)

percentage = 100 * inside / len(dt)

print("\n========== 3 SIGMA ==========\n")

print(
    f"Samples inside ±3σ : "
    f"{percentage:.3f}%"
)