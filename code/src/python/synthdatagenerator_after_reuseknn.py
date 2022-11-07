from sdv.tabular import GaussianCopula
from sdv.tabular import CTGAN
from sdv.evaluation import evaluate

import numpy as np
import pandas as pd

MAX_USERID = 71567
userid_mod = MAX_USERID + 1

data = pd.read_csv('ratings.csv')
row_count = len(data.index)

# Adding Laplacian noise
userid_noise = np.random.laplace(0,2,row_count)
ratings_noise = np.random.laplace(0,0.4,row_count)

# Changing the userids
for i in range(0, row_count):
    data['userid'][i] = (data['userid'][i] + round(userid_noise[i])) % userid_mod
    if (data['userid'][i] == 0):
        data['userid'][i] += 1

# Changing the ratings
for i in range(0, row_count):
    data['ratings'][i] = (data['ratings'][i] + round(ratings_noise[i])) % 6
    if (data['ratings'][i] == 0):
        data['ratings'][i] += 1

df = pd.DataFrame(data)
df.to_csv('out_laplacian.csv', index=False)

# Generating synthetic dataset
model_gauss = GaussianCopula()
model_ctgan = CTGAN()

model_gauss.fit(data)
model_ctgan.fit(data)

sample_gauss = model_gauss.sample(row_cout)
sample_gauss.head()
df = pd.DataFrame(sample_gauss)
df.to_csv('out_gauss.csv', index=False)

sample_ctgan = model_ctgan.sample(row_cout)
sample_ctgan.head()
df_ = pd.DataFrame(sample_ctgan)
df_.to_csv('out_ctgan.csv', index=False)

#print(evaluate(sample, data, metrics=['CSTest', 'KSTest'], aggregate=False))