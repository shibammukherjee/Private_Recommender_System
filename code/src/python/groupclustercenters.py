import matplotlib.pyplot as plt
import numpy as np
from sklearn.cluster import KMeans
from numpy import genfromtxt
import sys
import os
import glob


directory = sys.argv[1]
groupcount = int(sys.argv[2])
coloumnrange = int(sys.argv[3])

# ---- Make k mean of the cluster center dataset and group the nearest ones ----
my_data = genfromtxt(directory, delimiter=',')
kmeans = KMeans(n_clusters=groupcount)
kmeans.fit(my_data[:,:coloumnrange])

#print(kmeans.cluster_centers_)

# ---- Delete old files ----
files = glob.glob('../dataset/groupoutput/tempgroups/*')
for f in files:
    os.remove(f)

# ---- Writing the cluster centers in their appropriate groups ----
i=0
for l in kmeans.labels_:
    with open("../dataset/groupoutput/tempgroups/"+str(l)+".csv", "ab") as f:
        np.savetxt(f, np.resize(my_data[i,:], (1 , 4)), delimiter=",", fmt="%10.2f")
    i += 1

# ---- Writing the group centers in the file (We don't really need it i think) ----
#np.savetxt("../dataset/output/groupcenters.csv", kmeans.cluster_centers_, delimiter=",", fmt="%10.2f")

# ---- Showing the dataset with cluster centers in graph ----
# fig = plt.figure()
# ax = fig.add_subplot(111, projection='3d')
# ax.scatter(my_data[:,0],my_data[:,1],my_data[:,2], c=kmeans.labels_, cmap='rainbow', s=5)
# ax.scatter(kmeans.cluster_centers_[:,0] ,kmeans.cluster_centers_[:,1], kmeans.cluster_centers_[:,2], color='black', s=10)
# plt.show()