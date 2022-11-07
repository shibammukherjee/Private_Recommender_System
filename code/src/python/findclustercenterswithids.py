import matplotlib.pyplot as plt
import numpy as np
from sklearn.cluster import KMeans
from numpy import genfromtxt
import sys
import os
import glob
import random
import shutil
import re

from ctypes import c_uint32, c_uint64, sizeof

directory = sys.argv[1]
clustercount = int(sys.argv[2])
coloumnrange = int(sys.argv[3])
maximumclustersize = int(sys.argv[4])
clusterfileindex = 0
folderindex = 0

my_data = genfromtxt(directory, delimiter=',')
kmeans = KMeans(n_clusters=clustercount, n_init=10, max_iter=100)
kmeans.fit(my_data[:,:coloumnrange])

# ---- Removing the old files ----
files = glob.glob('../dataset/clusteroutput/tempclusters/*')
for f in files:
    os.remove(f)
files = glob.glob('../dataset/clusteroutput/*')
for f in files:
    try:
        os.remove(f)
    except:
        pass
files = glob.glob('../dataset/clusteroutput/clusters/*')
for f in files:
    os.remove(f)


# ---- Get the number of datapoints per cluster ----
print("Main clusters")
cluster_size = np.zeros(clustercount)
for l in kmeans.labels_:
    cluster_size[l] += 1
print(cluster_size)

# ---- Writing the cluster items in seperate temp files and adding the random id ----
i=0
for l in kmeans.labels_:
    with open("../dataset/clusteroutput/tempclusters/"+str(l)+".csv", "ab") as f:
        result = np.zeros((1,coloumnrange))
        #rand = random.randint(0,2**((sizeof(c_uint32)*8)-1))
        #result = np.append(my_data[i,:coloumnrange],rand)
        result = np.append(my_data[i,:coloumnrange],my_data[i,1])
        np.savetxt(f, np.resize(result, (1 , coloumnrange+1)), delimiter=",", fmt="%10.2f")
    i += 1


# ---- Check the max size and if the size exceeds perform kmean on the cluster to make their smaller clusters ----
print("Making subclusters of clusters bigger than clusters size: " + str(maximumclustersize))
index = 0
blacklistclusterindex = []
for i in cluster_size:
    if (i > maximumclustersize):
        blacklistclusterindex.append(index)
        cluster_data = genfromtxt("../dataset/clusteroutput/tempclusters/"+str(index)+".csv", delimiter=',')
        subclustercount = (int)(i/maximumclustersize)*2
        kmeans_ = KMeans(n_clusters=subclustercount, n_init=10, max_iter=100)
        kmeans_.fit(cluster_data[:,:coloumnrange])

        cluster_size_ = np.zeros(subclustercount)
        for l in kmeans_.labels_:
            cluster_size_[l] += 1
        print(cluster_size_)

        i=0
        try:
            shutil.rmtree("../dataset/clusteroutput/processingclusters/subcluster_"+str(folderindex))
        except:
            pass
        os.mkdir("../dataset/clusteroutput/processingclusters/subcluster_"+str(folderindex))
        for l in kmeans_.labels_:
            with open("../dataset/clusteroutput/processingclusters/subcluster_"+str(folderindex)+"/"+str(l)+".csv", "ab") as f:
                np.savetxt(f, np.resize(cluster_data[i], (1 , coloumnrange+1)), delimiter=",", fmt="%10.2f")
            i += 1

        folderindex += 1
    index += 1

# ---- Renaming and moving the sub cluster files to final clusteritems dir ----
maxfolders = folderindex
folderindex = 0
while(folderindex < maxfolders):
    files = sorted(glob.glob('../dataset/clusteroutput/processingclusters/subcluster_'+str(folderindex)+'/*'))
    files.sort(key=lambda f: int(re.sub('\D', '', f)))
    for f in files:
        os.rename(f,"../dataset/clusteroutput/clusters/"+str(clusterfileindex)+".csv")
        clusterfileindex += 1
    folderindex += 1

# ---- Writing the items of the accepted cluster in seperate files and adding the random id ----
i=0
try:
    shutil.rmtree("../dataset/clusteroutput/processingclusters/cluster")
except:
    pass
os.mkdir("../dataset/clusteroutput/processingclusters/cluster")
for l in kmeans.labels_:
    furthersubdivision = False
    for bl in blacklistclusterindex:
        if (l == bl):
            furthersubdivision = True
            break
    if (furthersubdivision == False):
        with open("../dataset/clusteroutput/processingclusters/cluster/"+str(l)+".csv", "ab") as f:
            result = np.zeros((1,coloumnrange))
            #rand = random.randint(0,2**((sizeof(c_uint32)*8)-1))
            #result = np.append(my_data[i,:coloumnrange],rand)
            result = np.append(my_data[i,:coloumnrange],my_data[i,1])
            np.savetxt(f, np.resize(result, (1 , coloumnrange+1)), delimiter=",", fmt="%10.2f")
    i += 1


# ---- Renaming and moving the accepted cluster files to final clusteritems dir ----
files = sorted(glob.glob('../dataset/clusteroutput/processingclusters/cluster/*'))
files.sort(key=lambda f: int(re.sub('\D', '', f)))
for f in files:
    os.rename(f,"../dataset/clusteroutput/clusters/"+str(clusterfileindex)+".csv")
    clusterfileindex += 1

# ---- Here we have +1 because the cluster id is incremental 
files = sorted(glob.glob('../dataset/clusteroutput/clusters/*'))
files.sort(key=lambda f: int(re.sub('\D', '', f)))
for f in files:
    rand = random.randint(0,2**((sizeof(c_uint32)*8)-1))
    my_data = genfromtxt(f, delimiter=',')
    kmeans = KMeans(n_clusters=1, n_init=10, max_iter=100)
    # ---- When the cluster has more than 1 point ----
    try:
        kmeans.fit(my_data[:,:coloumnrange])
        # ---- Giving id to each cluster center ----
        result = np.zeros((1,coloumnrange))
        index = 0
        for i in kmeans.cluster_centers_:
            result = np.append(kmeans.cluster_centers_[index],rand)
            index += 1
        # ---- Writing the cluster center to final cluster center file with random ids ----
        with open("../dataset/clusteroutput/clustercenters.csv", "ab") as fi:
            np.savetxt(fi, result.reshape((1,coloumnrange+1)), delimiter=",", fmt="%10.2f")
    # ---- When the cluster has same as 1 point ----
    except:
        # ---- Giving id to each cluster center ----
        result = np.zeros((1,coloumnrange))
        result = np.append(my_data[:coloumnrange],rand)
        # ---- Writing the cluster center to final cluster center file with random ids ----
        with open("../dataset/clusteroutput/clustercenters.csv", "ab") as fi:
            np.savetxt(fi, result.reshape((1,coloumnrange+1)), delimiter=",", fmt="%10.2f")
    
    # ---- We rename the file from serial index 0... to the random id that is provided to that cluster ----
    os.rename(f,"../dataset/clusteroutput/clusters/"+str(rand)+".csv")

