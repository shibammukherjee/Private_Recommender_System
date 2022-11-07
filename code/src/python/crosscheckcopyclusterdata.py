import sys
import glob
import os
from numpy import genfromtxt
import numpy as np
import re


clusterdirectory = sys.argv[1]
savefiledirectory = sys.argv[2]


# ---- Removing the old file ----
try:
    os.remove(savefiledirectory)
except:
    pass

open(savefiledirectory, "x")

files = sorted(glob.glob(clusterdirectory + "/*"))
files.sort(key=lambda f: int(re.sub('\D', '', f)))
for f in files:
    with open(savefiledirectory, "a") as dest, open(f, "r") as src:
        for line in src:
            dest.write(line)