import statistics as st
import math
import random as ra
# Copyright 2025 zawy, MIT license

# Usage: Just run on command line. Change variables below for testing.

# This simulates a PoW DAG using Bob McElrath's Braidpool scheme.
# https://github.com/braidpool/braidpool/blob/main/docs/braid_consensus.md
#
# It works like this: the difficulty algorithm tries to target 2.42 total blocks per consensus 
# aka cohort block as his paper describes it, which is a very simple scheme compared ot others.
# It doesn't use timestamps or estimate hashrate. The hashrate * latency cancels time units.
# The latency of the network can be thought of as the clock ticks.  Another way to view the lack of a 
# need for timestamps is that it's trying to go as fast as possible, getting consensus in only about
# 3x the latency.  
#
# Notation follows Bob's paper:
# a = median network latency
# L = lamdba = hashrate
# x = target = 2^256/difficulty
# Q = 0.7035 = axL which is the target for the fastest-possible consensus with smooth difficulty.
# 2.42 = 1+1/Q = Nb/Nc target

# max latency and current hashrate with random variability levels
a           = 1;   a_var = 0  # express a as MEDIAN latency
use_exact   = 1 ; 
if use_exact: print("Using median a as exact delay for all peers")
L           = 1 ;     L_var = 0 
Nb          = 100				# blocks for DAA
n           = 1					# mean lifetime of DAA filter
skip_recents = 0	# DAA ignores most recent blocks
Q           = 0.703467 

Nb_Nc_target = (1+1/Q)
blocks      = 8000	# number of  blocks in this simulation
Nc_adjust   = 0  # in case there is an error in most recent Nc count.

print("median a\tL\tNb\tn\t**\tQ\t\tblocks")
print("{:.2f}\t\t{:.1f}\t{:.0f}\t{:.0f}\t{:.0f}\t{:.4f}\t{:.0f}".format(a, L, Nb, n, skip_recents,Q, blocks))
print("** Most recent blocks skipped by DAA")

# need to redo everything in terms of numbpy arrays instead of lists to be ~ 5x faster

def prevent_incest(p):
   # try: len(parents[p]) 
   # except IndexError: return
    for q in range(len(parents[p])): # length of h-i's sublist ( parents[h-i][sublist]
        seen[parents[p][q]] = 1 # exclude p's parents, each stored as a height in element q of sublist
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
# initialize lists
not_a_common_ancestor = [0]*(blocks)
Nb_Nc = [0.0]*(blocks)
time = [None]*(blocks)
x = [0]*(blocks)
solvetime = [0]*(blocks)
num_parents = [0]*(blocks)

# parents[of h][are h's]  This will have sublists with variable length
parents = [[] for _ in range(blocks)]
# parents = [0]*(blocks)

# genesis block
x[0] = 1 
solvetime[0] = -1/x[0] * math.log(ra.random()) * (L + ra.uniform(-1.0*L_var,L_var))
time[0] = solvetime[0]
num_parents[0] = 0
Nb_Nc[0] = Nb_Nc_target 
parents[0].append(0)

# initialize 1st difficulty window
for h in range(1,Nb+skip_recents+20): # plus 20 because delay prevents some blocks from ebing seen
    x[h] = 1 
    solvetime[h] = -1/x[h-1] * math.log(ra.random()) * (L + ra.uniform(-1.0*L_var,L_var))
    time[h] = solvetime[h] + time[h-1] 
    num_parents[h] = 1 ; Nb_Nc[h] = Nb_Nc_target
    parents[h].append(h-1)

for h in range(Nb + skip_recents+20,blocks):
    # if h == 500: L *= 20.0
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
    for i in range(1,Nb + skip_recents + 20):
        next_i = 0
        delay = (ra.random()*a + ra.uniform(-a_var,a_var)) # a = median a
        if use_exact==1 : delay = a; 
        if delay > time[h] - time[h-i]:   # can't see block h-i due to delay
            not_a_common_ancestor[h-i] = 1  # remember all non-Nc blocks
        else:
            if len(Nb_blocks) < Nb+skip_recents: 
                Nb_blocks.append(h-i) # assume this equals hash-ordering
            prevent_incest(h-i)  # finds all recently-seen blocks

# Find parents. Assumes no parents older than difficulty window
    for i in range(Nb+skip_recents): 
        if seen[Nb_blocks[i]] == 0: parents[h].append(Nb_blocks[i]) # if seen is older than oldest Nb block, it "can't" be a parent
    try: num_parents[h] = len(parents[h])
    except IndexError: num_parents[h] = 0

# get x1, Nc, and Nb_Nc
    Nc=0
    if h < Nb + skip_recents+20: 
        x1 = 1
        Nb_temp = h
        Nc = h/Nb_Nc_target
    else:
        Nb_temp = Nb
        sum_x_inv = 0 ; Nc = 0
        x1=0
        for i in range(Nb):
            sum_x_inv += 1/x[Nb_blocks[i]]
            if not not_a_common_ancestor[Nb_blocks[i]]: Nc += 1
            x1 = Nb_temp/sum_x_inv  # harmonic mean
    if Nc == 0: Nc=1
    Nb_Nc[h] = Nb_temp/(Nc+Nc_adjust)
    
# 	calculate x[h]
    # need to do lookup table for CF = 0.7035 / W(Nb/Nc - 1) where W()=lambert function
    if Nb_Nc[h] < 1.7:  CF = 1.5
    elif Nb/Nc > 3.1:   CF = 0.66**(Nb_Nc[h] /5)
    else:               CF = Nb_Nc_target/(Nb_Nc[h])
    x[h] = x1*(1 + CF/n - 1/n)

# end generating blocks
print("             Mean  StdDev")
print("x:           {:.3f} {:.3f}".format(st.harmonic_mean(x), st.pstdev(x)))
print("solvetime:   {:.3f} {:.3f}".format(st.mean(solvetime), st.pstdev(solvetime)) )# 
print("Nb/Nc:       {:.3f} {:.3f} (2.42 goal)".format(st.mean(Nb_Nc), st.pstdev(Nb_Nc)))
print("num_parents: {:.3f} {:.3f}".format(st.mean(num_parents), st.pstdev(num_parents)))
print("Nc blocks: " + str(blocks - sum(not_a_common_ancestor)))
print("Time/Nc/a: {:.3f}".format(time[blocks-1]/(blocks - sum(not_a_common_ancestor))/a))
