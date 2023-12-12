import numpy as np

def generate_zipf_distribution(alpha, size):
    zipf_distribution = np.random.zipf(alpha, size=size)
    normalized_distribution = zipf_distribution
    return normalized_distribution

key_alpha = 2.0
value_alpha = 1.4
size = 25000000
filename = "zipf_4.dat"

np.random.seed (42)
key = generate_zipf_distribution(key_alpha, size)
value = generate_zipf_distribution(value_alpha, size)
value = value % 1000000

delta = dict ()
count = dict ()
vv = []

with open (filename, 'wb') as file:
    for k, v in zip (key, value):
        k = int (k)
        v = int (v)
        if k not in delta:
            delta [k] = int (np.max ([0, np.floor (np.random.normal (100000, 10000))]))
            count [k] = 0
        binary_k = k.to_bytes (13, 'little')
        d = int (v * 1000 + delta [k])
        count [k] = count [k] + 1
        binary_v = d.to_bytes (4, 'little')
        file.write (binary_k)
        file.write (binary_v)
        vv.append (d)

quantiles = [0.25, 0.5, 0.75, 0.85, 0.9, 0.95, 0.99]
print (quantiles)
print (np.quantile (vv, quantiles))
print (np.max (vv))
print (len (delta))

larger_than_30 = 0
sum_larger_than_30 = 0
larger_than_50 = 0
sum_larger_than_50 = 0
for j in count.values ():
    if j > 30:
        larger_than_30 += 1
        sum_larger_than_30 += j
    if j > 50:
        larger_than_50 += 1
        sum_larger_than_50 += j
print (f"larger than 30: {larger_than_30}, sum: {sum_larger_than_30}")
print (f"larger than 50: {larger_than_50}, sum: {sum_larger_than_50}")
