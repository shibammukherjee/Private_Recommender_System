import matplotlib.pyplot as plt
import numpy as np
from sklearn.cluster import KMeans
from numpy import genfromtxt
import sys
import os
import glob

my_data = genfromtxt(sys.argv[1], delimiter=',')

x = int(sys.argv[2])
y = int(sys.argv[3])
z = int(sys.argv[4])

for i in my_data[:,:3]:
    d = (x-i[0])**2 + (y-i[1])**2 + (z-i[2])**2
    print(d)