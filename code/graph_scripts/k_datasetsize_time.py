import numpy as np
import matplotlib.pyplot as plt

# k = [1, 2, 3, 4, 5, 7]
# old method
# size_old = [9500, 15500, 20300, 25027, 27800, 33000, 40348]
# time_old = [12, 14, 18, 24, 27, 31, 44]

# new method
size_new = [20000, 40000, 60000, 80000, 100000]
time_new = [9, 17, 21, 29, 35]

# For 3D
# fig = plt.figure()
# ax = plt.axes(projection='3d')
# ax.set_xlabel("K")
# ax.set_ylabel("Size")
# ax.set_zlabel("Time")
# ax.scatter3D(k, size, time, c='red', cmap='hsv')

# For 2D
# plt.plot(size_old, time_old, marker='o', linestyle='dashed', linewidth=2, markersize=8, label="old implementation")
plt.plot(size_new, time_new, marker='o', linestyle='dashed', linewidth=2, markersize=8, label="ReuseSANNS")
plt.xlabel("Size")
plt.ylabel("Time")
plt.legend()

plt.show()