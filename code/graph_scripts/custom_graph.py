import numpy as np
import matplotlib.pyplot as plt


size_new = [1000000, 10000000, 100000000]
time_standard = [6, 14, 17]
time_soc = [17, 53, 103]

memory = [1.9, 3.3, 4.4]

network = [66, 106, 146]

# For 2D
# plt.plot(size_new, time_standard, marker='o', linestyle='dashed', linewidth=2, markersize=8, label="Standard CPU")
# plt.plot(size_new, time_soc, marker='o', linestyle='dashed', linewidth=2, markersize=8, label="SOC CPU")
# plt.title("Size-Time Graph")
# plt.xlabel("Size")
# plt.ylabel("Time")
# plt.legend()
# plt.savefig('custom_1.png')

# plt.plot(size_new, memory, marker='o', linestyle='dashed', linewidth=2, markersize=8, label="Memory")
# plt.title("Size-Memory Graph")
# plt.xlabel("Size")
# plt.ylabel("Memory")
# plt.legend()
# plt.savefig('custom_2.png')

plt.plot(size_new, memory, marker='o', linestyle='dashed', linewidth=2, markersize=8, label="Network")
plt.title("Size-Network Graph")
plt.xlabel("Size")
plt.ylabel("Network")
plt.legend()
plt.savefig('custom_3.png')

#plt.show()