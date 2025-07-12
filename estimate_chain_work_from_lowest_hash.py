import math
import random
import statistics  # Added for median calculation

# This simulates the lifetime of blocks on to investigate the relation between the lowest hash
# seen and the total chain work.  See:
# https://github.com/zawy12/difficulty-algorithms/issues/84
#
# Difficulty = hashrate = 1 therefore difficulty / hashrate = blocktime = 1

trials = int(1000)
blocks = int(1000)
avg_hashes = 0
avg_implied_hashes = 0
hash_ratios = []
# The following 2 parameters were unnecessary, but make it complete.
# To simplify this code, set these 2 equal to 1 which can remove all occurences below. 
max_target = pow(2,256)
blocktime = 600
for j in range(trials):
    solvetime = 0
    lowest_hash = pow(2,256)
    hashes= 0
    implied_hashes = 0    
    factor = 1 
    for i in range(blocks):
        difficulty_target = max_target / 1000
        hashrate = max_target / difficulty_target / blocktime
        if i > blocks/2: # checking to see if hashrate chnaging has an effect. 
            difficulty_target *= 0.1
            hashrate = max_target/difficulty_target / blocktime 
        winning_hash_value = random.random() * difficulty_target # it's somewhere from 0 to target.
        lowest_hash = min(lowest_hash, winning_hash_value)
        solvetime = -math.log(random.random()) / difficulty_target / hashrate * max_target 
        hashes += solvetime * hashrate   #  Divided by max_target to match the github article
    implied_hashes = max_target / lowest_hash # Change max_target to 1 to match the article
    avg_implied_hashes += implied_hashes / trials
    avg_hashes += hashes / trials 
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
terminal_width = 120  # Adjust for terminal width

# Print histogram
print("\nHistogram of hash_ratios (0.05 increments):")
for bin_start in range(int(min_ratio / bin_width), int(max_ratio / bin_width) + 1):
    bin_value = bin_start * bin_width
    freq = bins.get(bin_value, 0)
    bar_length = int(freq * terminal_width / max_freq)
    print(f"{bin_value:.2f} | {'x' * bar_length} {freq}")

# Print results
print(f"chain_work/estimate using estimate = 2^256/6/lowest_hash): {avg_implied_hashes / 6 / avg_hashes}")
print(f"Finite sample bias correction: 2^256/lowest_hash/(ln(trials) +0.577): {avg_implied_hashes/avg_hashes/(math.log(trials) + 0.577)}")
print("Mean W/D: " + str(statistics.mean(hash_ratios)))  # should be 1.0 for exponential distribution
print("StdDev W/D: " + str(statistics.stdev(hash_ratios)))   # should be 1.0 for exponential distribution
print("Median W/D: " + str(statistics.median(hash_ratios))) # should be ln(2) = 0.693 for exponential distribution
