import math
import random
import statistics  # Added for median calculation

# This simulates the lifetime of blocks on to investigate the relation between the lowest hash
# seen and the total chain work.  See:
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
        hashes += solvetime*factor   # W in the article
    implied_hashes = 1 / lowest_hash # D in the article
    avg_hashes += hashes / trials
    avg_implied_hashes += implied_hashes / trials
    hash_ratios.append(hashes / implied_hashes)

# Histogram plotting
min_ratio = min(hash_ratios)
max_ratio = 6 # max(hash_ratios)
bin_width = 0.1
bins = {}
for ratio in hash_ratios:
    bin_index = int(ratio / bin_width) * bin_width
    bins[bin_index] = bins.get(bin_index, 0) + 1

# Find max frequency for scaling
max_freq = max(bins.values()) if bins else 1
terminal_width = 50  # Adjust for terminal width

# Print histogram
print("\nHistogram of hash_ratios (0.05 increments):")
for bin_start in range(int(min_ratio / bin_width), int(max_ratio / bin_width) + 1):
    bin_value = bin_start * bin_width
    freq = bins.get(bin_value, 0)
    bar_length = int(freq * terminal_width / max_freq)
    print(f"{bin_value:.2f} | {'x' * bar_length} {freq}")

# Print results
print(f"chain_work/estimate using estimate = 2^256/2/lowest_hash): {avg_implied_hashes / 2 / avg_hashes}")
print(f"chain_work/estimate using estimate = 2^256/6/lowest_hash): {avg_implied_hashes / 6 / avg_hashes}")
print("Mean W/D: " + str(statistics.mean(hash_ratios)))  # should be 1.0 for exponential distribution
print("StdDev W/D: " + str(statistics.stdev(hash_ratios)))   # should be 1.0 for exponential distribution
print("Median W/D: " + str(statistics.median(hash_ratios))) # should be ln(2) = 0.693 for exponential distribution
