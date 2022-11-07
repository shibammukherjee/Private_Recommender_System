import sys
import glob
import os
from numpy import genfromtxt
import numpy as np
import math

from ctypes import c_uint32, c_uint64, sizeof


def crosscheckfindtopk(n_dimension_ = None, originaldirectory_ = None, saveddirectory_ = None, q1_ = None, q2_ = None, q3_ = None, k_ = None):
    global isfunctionmode
    
    # 1 Part - Here we read the original dataset and make the modified comparision dataset in a different directory

    n_dimension = int(sys.argv[1])
    originaldirectory = sys.argv[2]
    savedirectory = sys.argv[3]

    # ---- Removing the old file ----
    try:
        os.remove(savedirectory)
    except:
        pass

    data = genfromtxt(originaldirectory, delimiter=',')
    data_ndim = data[:,:n_dimension]

    final_data = data_ndim.copy().tolist()

    for idx, item in enumerate(data_ndim):
        final_data[idx].append(item[1])


    with open(savedirectory, "wb") as f:
            np.savetxt(f, final_data, delimiter=",", fmt="%10.2f")
    ###################################################


    # Part 2 - Here we do the topk with mentioned in the paramter dataset to comapare it with the private HE implementation

    q1 = int(sys.argv[4])
    q2 = int(sys.argv[5])
    q3 = float(sys.argv[6])
    k = int(sys.argv[7])

    my_data = genfromtxt(savedirectory, delimiter=',')
    distanceidlist = []

    for idx, data in enumerate(my_data):
        dist = ((q1-data[0])**2 + (q2-data[1])**2 + (q3-data[2])**2)
        distanceidlist.append([dist, data[3]])

    # takes the first element of an array inside an array like [ [1,2], [1,2], ... ]
    def takefirst(elem):
        return elem[0]

    distanceidlist.sort(key=takefirst, reverse=False)

    for idx, d in enumerate(distanceidlist):
        if idx == k:
            break
        print("Distance", d[0], "ID", d[1])


crosscheckfindtopk()