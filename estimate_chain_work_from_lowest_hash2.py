import math
import random
import statistics  # Added for median calculation

# This simulates the lifetime of a blockchain to investigate the relation between the lowest hash
# seen and the total chain work.  See:
# https://github.com/zawy12/difficulty-algorithms/issues/84
#
# Two different formulas are check for accuracy over many trial runs:
# Work = max_target / average(lowest_hash_seen )
# and
# Work = average ( max_target / lowest_hash_seen )

trials = int(1000)
blocks = int(1000)
avg_hashes = 0
avg_lowest_hash = 0
avg_work_estimate = 0
hash_ratios = []
# The following 2 parameters were unnecessary, but make it complete.
# To simplify this code, set these 2 equal to 1 which can remove all occurences below. 
max_target = pow(2,256)
blocktime = 600
for j in range(trials):
    solvetime = 0
    lowest_hash = pow(2,256)
    hashes = 0
    implied_hashes = 0   
    difficulty_target = max_target / 100000
    for i in range(blocks):
        hashrate = max_target / difficulty_target / blocktime # assumes difficulty is set correctly for the hashrate
        winning_hash_value = random.random() * difficulty_target # it's somewhere from 0 to target.
        lowest_hash = min(lowest_hash, winning_hash_value)
        solvetime = -math.log(random.random()) * (max_target / difficulty_target) / hashrate  
        hashes += solvetime * hashrate   
    avg_work_estimate += max_target / lowest_hash / trials
    avg_lowest_hash += lowest_hash / trials
    avg_hashes += hashes / trials 

# Print results
print(f"(max_target/avg_lowest_hash) / hashes: {(max_target/avg_lowest_hash) / avg_hashes}")
print(f"average(max_target/lowest_hash) / hashes: {avg_work_estimate / avg_hashes}")
