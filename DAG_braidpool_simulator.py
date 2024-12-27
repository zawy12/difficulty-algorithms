# DAG PoW difficulty algorithm simulator.

# Copyright 2025 zawy, MIT license

# This command-line script tests two different difficulty adjustment algorithms (DAA's) for a DAG PoW with fast consensus. 
# They are remarkable in not needing timestamps or measurements of hashrate or latency. 

# Discussion is at the end of this file.

# Usage: Run on command line to see the output. Change variables below for testing.

import statistics as st
import math
import time as timer
import random as ra
import sys
import numpy as np
use_approx_DAA = 0
try: from scipy.special import lambertw
except ModuleNotFoundError: use_approx_DAA = 1
start = timer.time()

blocks      = 10000	    # number of  blocks in this simulation
a           = 1         # a = latency. Express 'a' as a MEDIAN and MEAN latency. Simulator randomnly varies from 0 to 2 * a 
a_var       = 0         #  a_var allows additional random variation
use_exact_latency = 1   # select this = 1  to make latency for every peer exactly 'a' instead of a median.
L           = 1         # L = lambda = hashrate. 
n           = 100	    # Filter for Nb/Nc DAA. Mean lifetime of DAA filter to smooth variations and slow it down. 100 is good. 
N           = 100       # Filter for parent DAA, needs to be Nb*n/2 for comparison to Nb/Nc DAA
use_parents = 1        # = 1 tells program to use parents DAA instead of Nb/Nc DAA
parents_desired = 2     # 2 for this value gives fastest time to consensus which esults in Nb/Nc of 2.718 May not be accurate above 5.
Nb              = 50      # Blocks for Nb/Nc DAA to look into the past to count Nc blocks. At least 20 unless Nc_adjust is used
if use_parents:
    Nb = parents_desired * 4 # Parent DAA needs to look back about this many Nb blocks to find all parents
Nc_adjust       = 0.0      # to be more accurate, can be on the order of 0.5 instead of 0 if Nb is smaller than 15
Q               = 0.703467     # 0.703467 is Bob's theoretical optimal setting of Q = a*x*L to get fastest blocks. This value is solution to Q=e^(-Q/2). 0.78 is the value that gives about 2 parents. 
Nb_Nc_target    = (1+1/Q)  # This is 2.4215 from the above Q, but now 2.71828 appears better. $Results in 2.718x latency time per consensus

use_attack          = 0     # if = 1, use on-off mining attack 
attack_length       = 100   # number of blocks for 1 on-off cycle
attack_size         = 2     # multiplier for the increase in hashrate
if use_parents and use_attack:
    Nb *= attack_size       # have to look back further to find parents if hashrate suddenly increases
    
use_latency_change  = 0     # if = 1, same kind of "attack" on latency. 
a_change_length     = 100   # number of blocks for 1 on-off cycle
a_change_size       = 2     # multiplier for the increase in latency
if use_parents and use_latency_change:
    Nb *= a_change_size     # have to look back further to find parents if latency suddenly increases    

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

data = [['latency a','hashrate L','Nb','Nb/Nc desired','filter n', 'blocks']]

if use_exact_latency: print("\nUser chose to use 'a' as the exact latency between every peer.\n")
if use_attack: 
    print("\nUser selected an on-off attack.")
    print("Attack_size = {:.2f}, Attack_length = {:.2f}".format(attack_size, attack_length))
if use_latency_change:
    print("User selected changes in latency.")
    print("Latency change size = {:.2f}, Latency change length = {:.2f}".format(a_change_size, a_change_length))
    
if use_parents == 1: 
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
    
if (n < 10) and use_parents == 1: print("\n===== The parent DAA may need N > 10.\n")

# initialize Python lists
Nb_blocks =  np.zeros(Nb, dtype=np.int32) # array of 0's and integers
not_a_common_ancestor = [0]*(blocks)
Nc_blocks = [0]*(blocks)
seen = np.zeros(blocks, dtype=np.int32)
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

# initialize 1st difficulty window plus a buffer for siblings
for h in range(1, Nb+20): 
    x[h] = 1 ; Nb_Nc[h] = Nb_Nc_target ; num_parents[h] = 1 ; parents[h].append(h-1)
    solvetime[h] = get_solvetime(x[h-1], L)
    time[h] = solvetime[h] + time[h-1] 
    
##### START MINING #######
attack_on = 0
latency_change_on = 0
for h in range(Nb+20,blocks): # start creating blocks & examinations after initialization
    if use_latency_change:
        if h > a_change_length and h%a_change_length == 0:
            if a_change_on == 0:
                a *= a_change_size ; a_change_on = 1
            else: 
                a /= a_change_size ; a_change_on = 0
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
    # print(h)
	
# for this block, find non-Nc blocks (siblings), parents, and avoid incest
    seen.fill(0)
    Nb_blocks.fill(0)
    i = 0 # keeps track of how many Nb blocks found (non-siblings)
    k = 1
    while i < Nb: # Nb ("look-back" blocks) has been changed to 8*parents_desired if using parent DAA
        
        latency = (ra.random()*2*a + ra.uniform(-a_var,a_var)) # 2x assumes user assigned a = median.
        if use_exact_latency == 1 : latency = a
        if latency > time[h] - time[h-k]:   # Can't see block h-k due to latency
        # Remember non-Nc blocks. A cheat to determine Nc blocks using our global knowledge of time, order, & latency.
            not_a_common_ancestor[h-k] = 1
        else:
            # Combined with the above 'if', determine if previous block was an Nc consensus block.
            # Occurs when latency before & after h-k-1 block was less than the before & after solvetimes. 
            # Assumes miners don't ignore any parent they saw.
            if not_a_common_ancestor[h-k-1] == 0: 
                Nc_blocks[h-k-1] = 1 # 1 = yes, this was an Nc block
            Nb_blocks[i] = h-k
                
            # Store parents of the current look-back block as seen[] (they can't be a parent of this block)
            for q in range(len(parents[h-k])):
                seen[parents[h-k][q]] = 1  # seen[height] = 1 means it not be a parent
            i += 1 # increment until we've looked back i = Nb blocks
        k += 1
        
    # Nb blocks in the past that are not siblings have been recorded. Now find which of these are
    # parents of current block. To be a parent, it has to be in Nb_blocks[] (not a sibling) and not 
    # in seen[]. This assumes no parents are older than Nb window.
    for j in range(Nb): 
        if seen[Nb_blocks[j]] == 0: 
            parents[h].append(Nb_blocks[j]) # this is a parent bc no other potential parents saw it.
    num_parents[h] = len(parents[h])

# Using parents DAA.
    if use_parents == 1:
        sum_x_inv = 0
        # calculate harmonic mean of parents' targets
        for m in range(num_parents[h]):
            sum_x_inv += 1/x[parents[h][m]]
        x1 = num_parents[h]/sum_x_inv
        x[h] = x1*(1 + (parents_desired - num_parents[h])/N)
            
# Using Nb/Nc DAA. 
    else:
        Nc=0 ; sum_x_inv = 0 ; x1=0
        for p in range(Nb):
            sum_x_inv += 1/x[Nb_blocks[p]]
            Nc += Nc_blocks[Nb_blocks[p]]
            
        x1 = Nb/sum_x_inv  # harmonic mean
        Nc = max(0.1, Nc)  # prevent divide by zero
        Nb_Nc[h] = Nb/(Nc-Nc_adjust)

        x[h] = x1*(1 + (Nb_Nc_target - Nb_Nc[h])/n)
# end generating blocks

print("             Mean  StdDev")
print("x:           {:.3f} {:.3f} (Try to get smallest SD/mean for a given Nb*n.)".format(st.harmonic_mean(x), st.pstdev(x)))
print("solvetime:   {:.3f} {:.3f}".format(st.mean(solvetime), st.pstdev(solvetime)) )# 
print("Nb/Nc:       {:.3f} {:.3f} ".format(blocks/sum(Nc_blocks), st.pstdev(Nb_Nc)))
print("num_parents: {:.3f} {:.3f}".format(st.mean(num_parents), st.pstdev(num_parents)))
print("Nc blocks: " + str(blocks - sum(not_a_common_ancestor)))
print("Time/Nc/a: {:.3f} (The speed of consensus as a multiple of latency.)".format(time[blocks-1]/(sum(Nc_blocks))/a))

print(f"\nRun time: {timer.time() - start:.2f} seconds")

sys.exit()

count = [0]*(10)
for i in range(blocks):
    for j in range(10):
        if num_parents[i] == j: count[j] +=1
for i in range(1,10):
    print(str(i) + " = " + str(count[i]/blocks) + "  " + str(count[i]) + "  " + str(count[1]/max(.1,count[i])))

'''
This simulates two PoW DAA's for Bob McElrath's Braidpool DAG. These DAA's target a DAG width without using timestamps or estimating hashrate or latency. This is possible because increasing hashrate or latency causes DAG width to increase to the same extent when the DAG is made as fast as possible. Increasing difficulty has the same effect on both and makes it narrow again. In setting target * hashrate * latency, time cancels. Consensus on Braidpool's scheme is achieved when the DAG width is 1, i.e. all parents and children of a certain "generation" of blocks see only 1 block in that generation which makes it easy to identify and have consensus for everyone to know the current state. This 1-block width occurs only if the difficulty is easy enough for the current hashrate so that there have not been any blocks mined before or after it within 1 latency period so that all honest parent and child blocks have time to see it. But for this 1-block consensus to occur as fast as possible, the difficulty also needs to be as easy as possible. Bob's Braidpool article discusses the math to try to identify the optimum solve rate to balance these two opposing effects to find the optimum difficulty: 
https://github.com/braidpool/braidpool/blob/main/docs/braid_consensus.md 

It turns out the fastest consensus occurs when Bob's Nb/Nc metric for DAG width = 2.71828. While running this simulator to try to test his math, I discovered simply targeting an average average number of parents. It turns out a mean of 2.000 parents gives the fastest consensus, and the StdDev of parents is 1.0000. This results in Bob's Nb/Nc ratio being 2.71828 and the average time between consensus is 2.71828 times the latency. To be clear, to set a target (2^256 / difficulty) for a particular block being mined, the consensus rule is that he and future validators only count his parents (which will typically be from 1 to 4) and get the harmonic mean of their targets and then use the equation below. This is incredibly simple compared to other DAGs and it may be the fastest possible consensus.

DAA using the number of parents:
-------------------------------
x = x1*(1 + (parents_desired - parents_observed)/n)
x1 = harmonic mean of the parent's targets
For comparison, n should be about Nb/2 times larger than in the Nb/Nc algorithm for a fair comparion because Nb/Nc DAA 
looks at Nb blocks in the past instead of 1. 

DAA using Nb/Nc:
---------------
x = x1*(1 + (Nb_Nc_desired - Nb_Nc_measured)/n)
x1 = harmonic mean of past Nb blocks' targets
'''
