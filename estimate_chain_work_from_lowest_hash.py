import math
import random
import statistics  # Added for median calculation

# This simulates the lifetime of blocks on to investigate the relation between the lowest hash
# seen and the total chain work.  See issue number 84:
# https://github.com/zawy12/difficulty-algorithms/issues/84

trials = int(1000)  
blocks = int(1000)
avg_hashes = 0
avg_implied_hashes = 0
hash_ratios = []
for j in range(trials):
    solvetime = 0
    lowest_hash = 1
    hashes= 0
    implied_hashes = 0    
    factor = 1 
    for i in range(blocks):
        if i > blocks/2: factor = 2 # adjustment for increasing hashrate
        hash_value = random.random()  
        lowest_hash = min(lowest_hash, hash_value/factor)
        solvetime = -math.log(random.random())  # this assumes difficulty matches hashrate for consistent blocktimes
        hashes += solvetime*factor
    implied_hashes = 1 / lowest_hash # / 2  for testing other equation
    avg_hashes += hashes / trials
    avg_implied_hashes += implied_hashes / trials
    hash_ratios.append(hashes / implied_hashes)

# Print results
print(f"Overestimate using W = 2^256/2/lowest_hash): {avg_implied_hashes / 2 / avg_hashes}")
print("Mean: " + str(statistics.mean(hash_ratios)))  # should be 1.0 for exponential distribution
print("Median: " + str(statistics.median(hash_ratios))) # should be ln(2) = 0.693 for exponential distribution
print("StdDev: " + str(statistics.stdev(hash_ratios)))   # should be 1.0 for exponential distribution
