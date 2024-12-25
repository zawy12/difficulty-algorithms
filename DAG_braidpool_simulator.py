import statistics as st
import math
import random as ra
import sys
use_approx_DAA = 0
try: from scipy.special import lambertw
except ModuleNotFoundError: use_approx_DAA = 1
try: import plotext
except ModuleNotFoundError: plotext_not_found = 1

# Copyright 2025 zawy, MIT license

# This command-line script tests two different difficulty adjustment algorithms (DAA's) for the fastest-possible DAG PoWs. 
# They are remarkable in not needing timestamps or measurements of hashrate or latency. 

# Usage: Run on command line to see the output. Change variables below for testing.

blocks      = 100	    # number of  blocks in this simulation
a           = 1         # a = latency. Express 'a' as a MEDIAN and MEAN latency. Simulator randomnly varies from 0 to 2 * a 
a_var       = 0         #  a_var allows additional random variation
use_exact_latency = 1   # select this = 1  to make latency for every peer exactly 'a' instead of a median.
L           = 1         # L = lambda = hashrate. 
n           = 100	    # Filter for Nb/Nc DAA. Mean lifetime of DAA filter to smooth variations and slow it down. 100 is good. 
N           = 150       # Filter for parent DAA, needs to be Nb*n/2 for comparison to Nb/Nc DAA
use_parents = 0         # = 1 tells program to use parents DAA instead of Nb/Nc DAA
parents_desired = 2     # 2 for this value gives fastest time to consensus which esults in Nb/Nc of 2.718 May not be accurate above 5.
Nb              = 10        # Blocks for Nb/Nc DAA to look into the past to count Nc blocks. At least 15 unless Nc_adjust is used
Nc_adjust       = 0.0        # to be more accurate, can be on the order of 0.5 instead of 0 if Nb is smaller than 15
Q               = 0.703467 # 0.58181977 # 0.703467  # Bob's theoretical optimal setting of Q = a*x*L to get fastest blocks. This value is solution to Q=e^(-Q/2). 
Nb_Nc_target    = (1+1/Q)  # This is 2.4215 from the above Q, but now 2.71828 appears better. $Results in 2.718x latency time per consensus
buffer          = 20 # leave at 20.  if too small like 5, this line has list out of range: if seen[Nb_blocks[i]] == 0. Values about 20 also bad.

use_attack          = 0 # if = 1, use on-off mining attack 
attack_length       = 100 # multiplier for the increase in hashrate
attack_size         = 2 # number of blocks for 1 on-off cycle

use_latency_change  = 0 # if = 1, same kind of "attack" on latency. 
a_change_length     = 100
a_change_size       = 2

'''
This simulates two PoW DAA's for Bob McElrath's Braidpool DAG. These DAA's target a DAG width without using timestamps or estimating hashrate or latency. This is possible because increasing hashrate or latency causes DAG width to increase to the same extent when the DAG is made as fast as possible. Increasing difficulty has the same effect on both and makes it narrow again. In setting target * hashrate * latency, time cancels. Consensus on Braidpool's scheme is achieved when the DAG width is 1, i.e. all parents and children of a certain "generation" of blocks see only 1 block in that generation which makes it easy to identify and have consensus for everyone to know the current state. This 1-block width occurs only if the difficulty is easy enough for the current hashrate so that there have not been any blocks mined before or after it within 1 latency period so that all honest parent and child blocks have time to see it. But for this 1-block consensus to occur as fast as possible, the difficulty also needs to be as easy as possible. Bob's Braidpool article discusses the math to try to identify the optimum solve rate to balance these two opposing effects to find the optimum difficulty: 
https://github.com/braidpool/braidpool/blob/main/docs/braid_consensus.md 

It turns out the fastest consensus occurs when Bob's Nb/Nc metric for DAG width = 2.71828. While running this simulator to try to test his math, I discovered simply targeting an average average number of parents. It turns out a mean of 2.000 parents gives the fastest consensus, and the StdDev of parents is 1.0000. This results in Bob's Nb/Nc ratio being 2.71828 and the average time between consensus is 2.71828 times the latency. To be clear, to set a target (2^256 / difficulty) for a particular block being mined, the consensus rule is that he and future validators only count his parents (which will typically be from 1 to 4) and get the harmonic mean of their targets and then use the equation below. This is incredibly simple compared to other DAGs and it may be the fastest possible consensus.

DAA using the number of parents:
-------------------------------
x = x1*(1 + 2/n - parents_observed/n)
x1 = harmonic mean of the parent's targets
For comparison, n should be about Nb/2 times larger than in the Nb/Nc algorithm for a fair comparion because Nb/Nc DAA 
looks at Nb blocks in the past instead of 1. 

DAA using Nb/Nc:
---------------
x = x1*(1 + 2.718/n - Nb/Nc/n)
x1 = harmonic mean of past Nb blocks' targets
    
'''
def print_table(data):   
    col_widths = [max(len(str(row[i])) for row in data) for i in range(len(data[0]))]
    for i, col in enumerate(data[0]):
        print(f"{col:<{col_widths[i]}}", end="  ")
    # print("\n" + "-" * (sum(col_widths) + 3 * (len(col_widths) - 1)))
    print()
    for row in data[1:]:
        for i, value in enumerate(row):
            print(f"{str(value):<{col_widths[i]}}", end="  ")
        print()

if use_exact_latency: print("\nUser chose to use 'a' as the exact latency between every peer.\n")
if use_attack: 
    print("\nUser selected an on-off attack.")
    print("Attack_size = {:.2f}, Attack_length = {:.2f}".format(attack_size, attack_length))
if use_latency_change:
    print("User selected changes in latency.")
    print("Latency change size = {:.2f}, Latency change length = {:.2f}".format(a_change_size, a_change_length))
    
if use_parents == 1:
    Nb = parents_desired * 7 # needed for looking back but low value makes it faster. 
    print("=====   User selected parent-count DAA. =====")
    print("x = x_parents_harmonic_mean * ( 1 + desired_parents/N - num_parents_measured/N)\n")
    data = [['latency a','hashrate L','parents desired','filter N', 'blocks'],
            [a,L,parents_desired,N, blocks]]
    print_table(data)
else: 
    if use_approx_DAA: print("scipy module is not installed. using approx method of Nb/Nc DAA.")
    print("=====   User selected Nb/Nc ratio DAA. Q=0.7035 =====")
    print("x = x_Nb_harmonic_mean * ( 1 + Nb_Nc_desired/n - Nb/Nc/n)\n")
    Nb_Nc_temp = int(Nb_Nc_target*10000)/10000
    data = [['latency a','hashrate L','Nb','Nb/Nc desired','filter n', 'blocks'],
            [a,L,Nb,Nb_Nc_temp,n,blocks]]
    print_table(data)
    
if (Nb < 10 or n < 10 or buffer < 10) and use_parents == 1:
    print("\n===== The parent DAA may cause a divide by zero in solvetime or list index out of range in seen[] if Nb, n, and buffer are smaller than 10.\n")


def remember_ancestors(p):
    try: len(parents[p])
    except IndexError: return
    for q in range(len(parents[p])):
        seen[parents[p][q]] = 1  # seen[height] = 1
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

def get_solvetime(xx,LL):
    return -1/xx * math.log(ra.random()) * LL

# Genesis block
x[0] = 1 ; Nb_Nc[0] = Nb_Nc_target ; num_parents[0] = 0 ; parents[0].append(0)
solvetime[0] = get_solvetime(x[0], L)
time[0] = solvetime[0]

# initialize 1st difficulty window
for h in range(1, Nb + buffer): # buffer because delay prevents some blocks from deing seen
    x[h] = 1 ; Nb_Nc[h] = Nb_Nc_target ; num_parents[h] = 1 ; parents[h].append(h-1)
    solvetime[h] = get_solvetime(x[h-1], L)
    time[h] = solvetime[h] + time[h-1] 
    
##### START MINING #######
attack_on = 0
latency_change_on = 0
for h in range(Nb+buffer,blocks):

    if use_attack:
        if h > attack_length and h%attack_length == 0:
            if attack_on == 0:
                L*=attack_size ; attack_on = 1
            else: 
                L =L/attack_size ; attack_on = 0
    
	# The following solvetime cheat using x[h-1] instead of x[h] prevents complexity. 
	# We don't know x[h] unless we know parents. But parents change as solvetime
	# increases. The solution would be to calculate delays, find parents, get x[h], 
	# hash a little which gets closer to the delays, find parents again, ... repeat.
    solvetime[h] = get_solvetime(x[h-1],L)
    time[h] = solvetime[h] + time[h-1]   	
	
# for this block, find non-Nc blocks (siblings), parents, and avoid incest
    seen = [0]*(blocks)
    Nb_blocks = []
    for i in range(1,Nb + buffer): # must be less than 20 blocks within 1 latency
        next_i = 0
        if use_latency_change:
            if h > a_change_length and h%a_change_length == 0:
                if a_change_on == 0:
                    a *= a_change_size ; a_change_on = 1
                else: 
                    a /= a_change_size ; a_change_on = 0
                    
        latency = (ra.random()*2*a + ra.uniform(-a_var,a_var)) # 2x assumes user assigned a = median.
        if use_exact_latency == 1 : latency = a
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
            remember_ancestors(h-i)  # This function identifies recently-seen blocks in 'seen' list.

# Find parents = blocks in past Nb blocks that were only seen by me. Assumes no parents are older than Nb window.
    for i in range(Nb): 
        if seen[Nb_blocks[i]] == 0: 
            parents[h].append(Nb_blocks[i])
    num_parents[h] = len(parents[h])

# Using parents  method to calculate x instead of Nb/Nb = 2.42 method.
    if use_parents == 1:
        sum_x_inv = 0
        for i in range(num_parents[h]):
            sum_x_inv += 1/x[parents[h][i]]
        x1 = num_parents[h]/sum_x_inv
        x[h] = x1*(1 + (parents_desired - num_parents[h])/N)
            
# Using Nb/Nc method. Get x1 (harmonic mean of targets), Nc, and Nb_Nc for difficulty calculation
    else:
        Nc=0 ; sum_x_inv = 0 ; x1=0
        for i in range(Nb):
            sum_x_inv += 1/x[Nb_blocks[i]]
            Nc += Nc_blocks[Nb_blocks[i]]
            
        x1 = Nb/sum_x_inv  # harmonic mean
        Nc = max(0.1, Nc) # prevent divide by zero
        Nb_Nc[h] = Nb/(Nc-Nc_adjust)

        if use_approx_DAA:
            if Nb_Nc[h] < 1.5:  CF = 2
            elif Nb_Nc[h] > 3.1:   CF = 0.66**(Nb_Nc[h] /5)
            else:               
                CF = Nb_Nc_target/(Nb_Nc[h])        
        else: CF = Q/max(0.1,lambertw(complex(max(.1,Nb_Nc[h]-1)),k=0).real)   

        x[h] = x1*(1 + CF/n - 1/n)
    print(h, parents[h], x[h], time[h])
# end generating blocks

print("             Mean  StdDev")
print("x:           {:.3f} {:.3f} (Try to get smallest SD/mean for a given Nb*n.)".format(st.harmonic_mean(x), st.pstdev(x)))
print("solvetime:   {:.3f} {:.3f}".format(st.mean(solvetime), st.pstdev(solvetime)) )# 
print("Nb/Nc:       {:.3f} {:.3f} ".format(blocks/sum(Nc_blocks), st.pstdev(Nb_Nc)))
print("num_parents: {:.3f} {:.3f}".format(st.mean(num_parents), st.pstdev(num_parents)))
print("Nc blocks: " + str(blocks - sum(not_a_common_ancestor)))
print("Time/Nc/a: {:.3f} (The speed of consensus as a multiple of latency.)".format(time[blocks-1]/(sum(Nc_blocks))/a))
