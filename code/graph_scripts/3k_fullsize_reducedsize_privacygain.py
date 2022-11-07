import numpy as np
import matplotlib.pyplot as plt

fig = plt.figure()
ax = plt.axes(projection='3d')

fullsize = [10000, 20000, 40000]
reducedsize = [5000, 8400, 11300]
privacygain = [65.41, 77.36, 89.24]

ax.set_xlabel("Full Size")
ax.set_ylabel("Reduced Size")
ax.set_zlabel("Privacy Improvement")

ax.scatter3D(fullsize, reducedsize, privacygain, c='red', cmap='hsv')


plt.show()