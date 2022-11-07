from sdv.tabular import GaussianCopula
from sdv.tabular import CTGAN
from sdv.evaluation import evaluate

import numpy as np
import pandas as pd

MAX_MOVIEID = 1682
movieid_mod = MAX_MOVIEID + 1

#data = pd.read_csv('ratings.csv')
#row_count = len(data.index)

# Adding Laplacian noise
movieid_noise = np.random.laplace(0,4,100000) # sensitivity -> 1 Ep -> ~0.25
ratings_noise = np.random.laplace(0,0.3,100000) # sensitivity -> 1 Ep -> ~2.7

count = 0
for i in ratings_noise:
    if i > 3:
        count += 1
print(count)


exit()


# Changing the userids
for i in range(0, row_count):
    data['movieid'][i] = (data['movieid'][i] + round(movieid_noise[i])) % movieid_mod
    if (data['movieid'][i] == 0):
        data['movieid'][i] += 1

# Changing the ratings
for i in range(0, row_count):
    data['ratings'][i] = (data['ratings'][i] + round(ratings_noise[i])) % 6
    if (data['ratings'][i] == 0):
        data['ratings'][i] += 1

df = pd.DataFrame(data)
df.to_csv('out_laplacian.csv', index=False)

exit()

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