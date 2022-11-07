import matplotlib.pyplot as plt
import numpy as np
from sklearn.cluster import KMeans
from numpy import genfromtxt
import sys
import os
import glob


file_data = genfromtxt(sys.argv[1], delimiter=',')

data = file_data[:,int(sys.argv[2])]
count = 0
for d1 in data:
    file_data[count, int(sys.argv[2])] = d1 * int(sys.argv[3])
    count += 1

np.savetxt(sys.argv[1][:-4] + "_normalized.csv", file_data, delimiter=",", fmt="%d")

    