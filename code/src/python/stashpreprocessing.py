import matplotlib.pyplot as plt
import numpy as np
from sklearn.cluster import KMeans
from numpy import genfromtxt
import sys
import os
import glob
import re

mingroupsize = int(sys.argv[1])

# ---- Delete old files ----
files = glob.glob('../dataset/groupoutput/*')
for f in files:
    try:
        os.remove(f)
    except:
        pass
files = glob.glob('../dataset/groupoutput/groups/*')
for f in files:
    try:
        os.remove(f)
    except:
        pass

# ---- Check if a group comes under stash, then put it in the stash file else put the groups in the final group list directory
files = sorted(glob.glob('../dataset/groupoutput/tempgroups/*'))
files.sort(key=lambda f: int(re.sub('\D', '', f)))
fileindex = 0
for f in files:
    my_data = genfromtxt(f, delimiter=',')

    # Checks if it is a single item in the group (will have just 1 dimension), (GOES TO STASH) --> 1, 52671, 5 is treated as 1 dimension for the parser
    if (len(my_data.shape) == 1):
        with open("../dataset/groupoutput/stash.csv", "ab") as f:
            cluster_data = genfromtxt("../dataset/clusteroutput/clusters/"+str(int(my_data[len(my_data)-1]))+".csv", delimiter=',')
            # Check if its a single item or not
            if(len(cluster_data.shape) == 1):
                np.savetxt(f, cluster_data.reshape(1, cluster_data.shape[0]), delimiter = ",", fmt="%10.2f")
            else:
                np.savetxt(f, cluster_data, delimiter = ",", fmt="%10.2f")
            
    # Checks if the size of the first dimension greater than minimum group size, that is the number cluster points is greater than minimum group size, (GOES TO STASH)
    elif(my_data.shape[0] < mingroupsize):
        with open("../dataset/groupoutput/stash.csv", "ab") as f:
            for d in my_data:
                cluster_data = genfromtxt("../dataset/clusteroutput/clusters/"+str(int(d[len(d)-1]))+".csv", delimiter=',')
                # Check if its a single item or not
                if(len(cluster_data.shape) == 1):
                    np.savetxt(f, cluster_data.reshape(1, cluster_data.shape[0]), delimiter = ",", fmt="%10.2f")
                else:
                    np.savetxt(f, cluster_data, delimiter = ",", fmt="%10.2f")

    # Else is a good group
    else:
        with open("../dataset/groupoutput/groups/"+str(fileindex)+".csv", "ab") as f:
            np.savetxt(f, my_data, delimiter=",", fmt="%10.2f")
        fileindex += 1
