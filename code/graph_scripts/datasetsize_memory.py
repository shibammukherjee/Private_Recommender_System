from cProfile import label
import numpy as np
import matplotlib.pyplot as plt

# old method
# dataitem_old = [9500, 15500, 20300, 24300, 27800, 33000, 40348, 100836]
# ram_old = [3.86, 4.7, 6.43, 7.89, 8.6, 10.85, 12.5, 54.6]

# old method
dataitem_new = [20000, 40000, 60000, 80000, 100000]
ram_new = [2.4, 5, 6.4, 7.7, 9]

# dataitem_1 = [40350, 100000]
# ram_1 = [12.5, 54.6]

# For 3D
# fig = plt.figure()
# ax = plt.axes(projection='3d')
# ax.set_xlabel("K")
# ax.set_ylabel("Size")
# ax.set_zlabel("Time")
# ax.scatter3D(k, size, time, c='red', cmap='hsv')

# For 2D
#plt.plot(dataitem, ram, label="Actual Result")
# plt.plot(dataitem_old, ram_old, marker='o', linestyle='dashed', linewidth=2, markersize=12, label="old implementation")
plt.plot(dataitem_new, ram_new, marker='o', linestyle='dashed', linewidth=2, markersize=12, label="ReuseSANNS")
plt.xlabel("Dataset Size")
plt.ylabel("Memory Required in GB")
plt.legend()
plt.show()