# Serilaizes a random dataset in accesdning order  based on index 0

from numpy import dtype, genfromtxt, index_exp
import numpy as np

dataset = genfromtxt("ml_100k_1700_movies.csv", delimiter=',')
# taking only the first n coloumns
n_coloumn = int(3)
dataset = dataset[:,:n_coloumn]

serialized_dataset = sorted(dataset, key=lambda x: x[0])

with open('ratings_sorted.csv', "wb") as f:
        np.savetxt(f, serialized_dataset, delimiter=",", fmt="%10.2f")