import statistics as st
import math
import random as ra
# Copyright 2025 zawy, MIT license

# Usage: Just run on command line. Change variables below for testing.

# This simulates a PoW DAG using Bob McElrath's Braidpool scheme:
# https://github.com/braidpool/braidpool/blob/main/docs/braid_consensus.md
#
# Verifying that either the theory or this code is correct is difficult, but 
# they get the same result without tweeking.
#
# It works like this: the difficulty algorithm tries to target 2.42 total blocks per consensus 
# aka cohort block as his paper describes it, which is a very simple scheme compared ot others.
# It doesn't use timestamps or estimate hashrate. The hashrate * latency cancels time units.
# The latency of the network can be thought of as the clock ticks.  Another way to view the lack of a 
# need for timestamps is that it's trying to mine blocks as fast as possible for a given DAG width, 
# so non-mining nodes are not placing a forward time limit block timestamps.
#
# Notation follows Bob's paper:
# -----------------------------
# a = median network latency
# L = lamdba = hashrate
# x = target = 2^256/difficulty
# Q = 0.7035 = axL which is the target for the fastest-possible consensus with smooth difficulty.
# 2.42 = 1+1/Q = Nb/Nc target

# Max latency and current hashrate with random variability levels
# a and L values can always be = 1. They just scale everything. Average x will just be lower if they're higher.
a           = 1; a_var = 0  # Express 'a' as a MEDIAN = MEAN latency. a_var allows additional random variation
use_exact   = 1 
if use_exact: print("User chose to use 'a' as the exact latency between every peer.")
L           = 1 ; L_var = 0  # L = lambda = hashrate. L_var allows additional random variation.
Nb          = 100    # blocks for DAA. A low Nb*n with correct Nb/Nc mean and low StdDev in Nc/Nc and x is the best DAA. 
Nc_adjust   = 0   # Increasing this above 0 may help if Nb is small, less than 20.  
n           = 1	  # mean lifetime of DAA filter, to smooth it out. Use 1 or more.
Q           = 0.703467 # theoretical optimal setting of Q = a*x*L to get fastest blocks that solves Q=e^(-Q/2) 
Nb_Nc_target = (1+1/Q)
blocks      = 5000	# number of  blocks in this simulation

print("median a\tL\tNb\tn\tQ\t\tblocks")
print("{:.2f}\t\t{:.1f}\t{:.0f}\t{:.0f}\t{:.4f}\t{:.0f}".format(a, L, Nb, n, Q, blocks))

def remember_ancestors(p):
    try: len(parents[p])
    except IndexError: return
    for q in range(len(parents[p])):
        seen[parents[p][q]] = 1 
        try: len(parents[q]) 
        except IndexError: break
        for r in range(len(parents[q])):
            seen[parents[q][r]] = 1
            try: len(parents[r]) 
            except IndexError: break
            for s in range(len(parents[r])):
                seen[parents[r][s]] = 1
                try: len(parents[s]) 
                except IndexError: break
                for t in range(len(parents[s])):
                    seen[parents[s][t]] = 1
                    try: len(parents[t]) 
                    except IndexError: break
                    for u in range(len(parents[t])):
                        seen[parents[t][u]] = 1
                        try: len(parents[u]) 
                        except IndexError: break
                        for v in range( len(parents[u])):
                            seen[parents[u][v]] = 1
# initialize Python lists
not_a_common_ancestor = [0]*(blocks)
Nc_blocks = [0]*(blocks)
Nb_Nc = [0.0]*(blocks)
time = [None]*(blocks)
x = [0]*(blocks)
solvetime = [0]*(blocks)
num_parents = [0]*(blocks)

# parents[of h][are h's]  This will have sublists with variable length
parents = [[] for _ in range(blocks)]

# Genesis block
x[0] = 1 ; Nb_Nc[0] = Nb_Nc_target ; num_parents[0] = 0 ; parents[0].append(0)
solvetime[0] = -1/x[0] * math.log(ra.random()) * (L + ra.uniform(-1.0*L_var,L_var))
time[0] = solvetime[0]

# initialize 1st difficulty window
for h in range(1,Nb+20): # plus 20 because delay prevents some blocks from deing seen
    x[h] = 1 ; Nb_Nc[h] = Nb_Nc_target ; num_parents[h] = 1 ; parents[h].append(h-1)
    solvetime[h] = -1/x[h-1] * math.log(ra.random()) * (L + ra.uniform(-1.0*L_var,L_var))
    time[h] = solvetime[h] + time[h-1] 
    
##### START MINING #######
for h in range(Nb+20,blocks):
    # if h == 500: L *= 20.0  # for testing sudden hashrate change
    num_parents[h]=0
	# The following solvetime cheat using x[h-1] instead of x[h] prevents complexity. 
	# We don't know x[h] unless we know parents. But parents change as solvetime
	# increases. The solution would be to calculate delays, find parents, get x[h], 
	# hash a little which gets closer to the delays, find parents again, ... repeat.
    solvetime[h] = -1.0/x[h-1] * math.log(ra.random()) * (L + ra.uniform(-1.0*L_var,L_var))
    time[h] = solvetime[h] + time[h-1]   	
	
# for this block, find non-Nc blocks (siblings), parents, and avoid incest
    seen = [0]*(blocks)
    Nb_blocks = []
    k = 0 
    for i in range(1,Nb + 20): # must be less than 20 blocks within 1 latency
        next_i = 0
        latency = (ra.random()*2*a + ra.uniform(-a_var,a_var)) # 2x assumes user assigned a = median.
        if use_exact==1 : latency = a
        if latency > time[h] - time[h-i]:   # Can't see block h-i due to latency
        # Remember all non-Nc blocks. A great cheat to get Nc using our global order & time knowledge.
        # Assumes every miner is honest. 
            not_a_common_ancestor[h-i] = 1
        else:
            # combined with the above, the following determines if the previous block was an Nc consensus blocks.
            # The latency before & after the h-i block was less than the 2 solvetimes.
            if not_a_common_ancestor[h-i-1] == 0: Nc_blocks[h-i-1] = 1
            if len(Nb_blocks) < Nb: 
                Nb_blocks.append(h-i) # This assumes hash-based ordering so validators see same list.
            remember_ancestors(h-i)  # This function finds all recently-seen blocks in 'seen' list.

# Find parents (no incest). Assumes no parents are older than difficulty window.
    for i in range(Nb): 
        if seen[Nb_blocks[i]] == 0: 
            parents[h].append(Nb_blocks[i])
    try: num_parents[h] = len(parents[h])
    except IndexError: num_parents[h] = 0

# Get x1 (harmonic mean of targets), Nc, and Nb_Nc for difficulty calculation
    Nc=0
    if h < Nb + 20: # just set defaults for 1st Nb blocks
        x1 = 1 ; Nb_temp = h ;  Nc = h/Nb_Nc_target
    else:
        Nb_temp = Nb
        sum_x_inv = 0 ; Nc = 0
        x1=0
        for i in range(Nb):
            sum_x_inv += 1/x[Nb_blocks[i]]
            Nc += Nc_blocks[Nb_blocks[i]]
        x1 = Nb_temp/sum_x_inv  # harmonic mean
    if Nc == 0: Nc=1 # prevent divide by zero
    Nb_Nc[h] = Nb_temp/(Nc+Nc_adjust)
    
# 	calculate x[h]
    # CF = 0.7035 / W(Nb/Nc - 1) where W()=lambert function
    # approximate method:
    '''
    if Nb_Nc[h] < 1.5:  CF = 2
    elif Nb/Nc > 3.1:   CF = 0.66**(Nb_Nc[h] /5)
    else:               CF = Nb_Nc_target/(Nb_Nc[h])
    '''
    # exact method:
    from scipy.special import lambertw
    CF = Q/max(0.01,lambertw(complex(max(.1,Nb_Nc[h]-1)),k=0).real)
    x[h] = x1*(1 + CF/n - 1/n) 
    
# end generating blocks

print("             Mean  StdDev")
print("x:           {:.3f} {:.3f} (Try to get smallest SD/mean for a given Nb*n.)".format(st.harmonic_mean(x), st.pstdev(x)))
print("solvetime:   {:.3f} {:.3f}".format(st.mean(solvetime), st.pstdev(solvetime)) )# 
print("Nb/Nc:       {:.3f} {:.3f} (2.42 is target with low SD.)".format(blocks/sum(Nc_blocks), st.pstdev(Nb_Nc)))
print("num_parents: {:.3f} {:.3f}".format(st.mean(num_parents), st.pstdev(num_parents)))
print("Nc blocks: " + str(blocks - sum(not_a_common_ancestor)))
print("Time/Nc/a: {:.3f} (The speed of consensus as a multiple of latency.)".format(time[blocks-1]/(sum(Nc_blocks))/a))
