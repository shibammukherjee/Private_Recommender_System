import numpy as np
import matplotlib.pyplot as plt

fig = plt.figure()
ax = plt.axes(projection='3d')

fullsize = [10000, 20000, 40000]
reducedsize = [4500, 7000, 9500]
privacygain = [74.04, 83.2, 90.54]

ax.set_xlabel("Full Size")
ax.set_ylabel("Reduced Size")
ax.set_zlabel("Privacy Improvement")

ax.scatter3D(fullsize, reducedsize, privacygain, c='red', cmap='hsv')


plt.show()