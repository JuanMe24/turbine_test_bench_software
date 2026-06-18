import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

df = pd.read_csv("encoder_data_stream.csv")

t = df["time_s"].to_numpy()

dt = np.diff(t)

plt.figure(figsize=(10,5))

plt.hist(
    dt*1000,
    bins=100
)

plt.xlabel("Ts [ms]")
plt.ylabel("Occurrences")
plt.title("Distribution of Sampling Period")

plt.grid(True)

plt.show()
 
plt.figure(figsize=(10,5))

plt.plot(dt*1000)

plt.xlabel("Sample")
plt.ylabel("Ts [ms]")
plt.title("Sampling Period")

plt.grid(True)

plt.show()