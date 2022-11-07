from cProfile import label
import numpy as np
import matplotlib.pyplot as plt

# time
matrix_aby = [10, 20, 30]
aby_time = [11.7, 85.6, 276.7]
matrix_tiny = [10, 20, 30, 40]
tiny_time= [1.2, 12.8, 41.8, 103.1]

# memory
# matrix_aby = [10, 20, 30]
# aby_mem = [1030, 8140, 27400]
# matrix_tiny = [10, 20, 30, 40]
# tiny_mem= [8.1, 12.2, 14, 18.6]

plt.plot(matrix_aby, aby_time, marker='o', linestyle='dashed', linewidth=2, markersize=12, label="aby")
plt.plot(matrix_tiny, tiny_time, marker='o', linestyle='dashed', linewidth=2, markersize=12, label="tinygarble2")
plt.xlabel("Matrix Dimension")
plt.ylabel("Time in Seconds")
plt.legend()

# plt.plot(matrix_aby, aby_mem, marker='o', linestyle='dashed', linewidth=2, markersize=12, label="aby")
# plt.plot(matrix_tiny, tiny_mem, marker='o', linestyle='dashed', linewidth=2, markersize=12, label="tinygarble2")
# plt.xlabel("Matrix Dimension")
# plt.ylabel("Memory in MB")
# plt.legend()

plt.show()