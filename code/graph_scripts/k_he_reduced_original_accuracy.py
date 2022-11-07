import numpy as np
import matplotlib.pyplot as plt

fig = plt.figure()
ax = plt.axes(projection='3d')

# k = 3, top-k = 5, 96.66, 43.33
# k = 3, top-k = 10, <100, 50
#-------------------------------------
#combining both
#-------------------------------------
# k = 3, ...., 97.83, 46.665

k = [3, 4, 7]
heaccuracycomapredtoreduceddataset = [97.83, 90, 95]
heaccuracycomapredtooriginaldataset = [46.665, 62.5, 45]

ax.set_xlabel("K")
ax.set_ylabel("Reduced Accuracy")
ax.set_zlabel("Original Accuracy")

ax.scatter3D(k, heaccuracycomapredtoreduceddataset, heaccuracycomapredtooriginaldataset, c='red', cmap='hsv')


plt.show()