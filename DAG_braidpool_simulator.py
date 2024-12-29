# DAG PoW difficulty algorithm simulator.

# Copyright 2025 zawy, MIT license

# This command-line script tests 3 different difficulty adjustment algorithms (DAA's) for a DAG PoW with fast consensus. 
# They are remarkable in not needing timestamps or measurements of hashrate or latency. 

# Discussion is at the end of this file.

# Usage: Run on command line to see the output. Change variables below for testing. The matplotlib.pyplot module 
# isn't a requirement but shows the plots.

import statistics as st
import math
import time as timer
import random as ra
import sys
import numpy as np
import matplotlib.pyplot as plt
# import networkx as nx

# from scipy.special import lambertw

start = timer.time()

DAA         = 1         # Which DAA? 0 = Nb/Nc, 1 = parents, 2 = SMA. 
blocks      = 20000	    # number of  blocks in this simulation
latency_    = .65       # base latency. Enter it as a MEDIAN or MEAN latency. Simulator can randomnly vary from 0 to 2a. 
latency_var = 0         # allows an additional random variation in latency. 
latency_2   = 1         # latency level #2 as a multiple or fraction of latency_. Set = 1 if every miner is the same. 
hrf_latency_2  = 0.25   # Hashrate fraction that has latency_2.

use_exact_latency = 1   # '1' to make latencies exactly 'a' or 'a2' instead of the median of 2*a & 2*a2 random variations.

hashrate_   = .66       # base hashrate, lambda. 
n0          = 100	    # Filter for Nb/Nc DAA. Mean lifetime of DAA filter to smooth variations and slow it down. 
n1          = 300       # Filter for parent DAA, needs to be Nb*n/7 for equal stability to Nb/Nc DAA
n2          = 660      # Filter for SMA
sma_block_time  = 1.44   # SMA block time T as a multiple of latency. 
parents_desired = 1.44  # 1.44 gives Nb/Nc =~ 2.42
Nb              = 40   # Blocks to look into the past. Can be pretty low as n0 is increased. 5 to 10 might be a lower limit. Going from 20 to 40 had a 33% reduction in Nb/Nc Std Dev (0.9 to 0.6) without regard to n0.
if DAA == 1: Nb = int(parents_desired * 3) # Parent DAA needs to look back about this many Nb blocks to find all parents
if DAA == 2: Nb = 20     # is needed to find parents and consensus blocks
Nc_adjust       = 1.3   # To make Nb/Nc DAA more accurate, this needs to be 1.3. Works for all settings.
Q               = 0.703467     # 0.703467 is Bob's theoretical optimal setting of Q = a*x*L to get fastest blocks. This value is solution to Q=e^(-Q/2).  
Nb_Nc_target    = (1+1/Q)   # This is 2.4215 from the above Q, but now 2.71828 appears better. $Results in 2.718x latency time per consensus

use_attack          = 1     # if = 1, use on-off mining attack 
attack_length       = 4000   # number of blocks for 1 on-off cycle
attack_size         = 2     # multiplier for the increase in hashrate
    
use_latency_change     = 1     # if = 1, same kind of "attack" on latency. 
latency_change_length  = 4000   # number of blocks for 1 on-off cycle
latency_change_size    = 2     # multiplier for the increase in latency

# The following 2 'ifs' are a cheat to prevent forks where PoW would select the winner in a real chain.    
if DAA == 1 and use_attack:
    Nb *= attack_size       # Look back further to find parents if hashrate suddenly increases
if DAA == 1 and use_latency_change:
    Nb *= latency_change_size     # Look back further to find parents if latency  suddenly increases. 

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

if use_exact_latency: print("\nUser chose to use exact latency instead of randomly varying around median.\n")
if use_attack: 
    print("\nUser selected an on-off attack.")
    print("Attack_size = {:.2f}, Attack_length = {:.2f}".format(attack_size, attack_length))
    
if use_latency_change:
    print("User selected changes in latency.")
    print("Latency change size = {:.2f}, Latency change length = {:.2f} blocks".format(latency_change_size, latency_change_length))
    
if DAA == 0: 
    print("=====   User selected Nb/Nc ratio DAA =====")
    print("x = x_harmonic_mean_of_Nb * ( 1 + (Nb_Nc_desired - Nb_Nc_observed)/n)\n")
    Nb_Nc_temp = int(Nb_Nc_target*10000)/10000
    data = [['latency','hashrate','Nb','Nb/Nc desired','filter n', 'blocks'],
            [latency_,hashrate_,Nb,Nb_Nc_temp,n0,blocks]]
elif DAA == 1: 
    print("=====   User selected parent-count DAA. =====")
    print("x = x_harmonic_mean_of_parents * ( 1 + (parents_desired - parents_measured)/n)\n")
    data = [['latency','hashrate','parents desired','filter n', 'blocks'],
            [latency_,hashrate_,parents_desired,n1, blocks]]
elif DAA == 2: 
    print("=====   User selected SMA DAA  =====")
    print("x = x_harmonic_mean_of_n * timespan_observed / timespan_desired\n")
    data = [['latency','hashrate', 'sma_block_time','n', 'blocks'],
            [latency_,hashrate_,sma_block_time,n2,blocks]]  

print_table(data)
    
if (n1 < 10) and DAA == 1: print("\n===== The parent DAA may need n1 > 10.\n")

# initialize arrays
Nb_blocks = np.zeros(Nb,     dtype=np.int32)   # Array of size Nb with 0's as integers. will store heights.
Nc_blocks = np.zeros(blocks, dtype=bool)   # index is height. True identifies a consensus block. 
seen      = np.zeros(blocks, dtype=bool)   # index is height. True identifies 'seen' blocks = it's not a parent block.
Nb_Nc     = np.zeros(blocks, dtype=float)  # The ratio Nb/Nc as observed by a block at height h
time      = np.zeros(blocks, dtype=float)  # Simulator has access to precise time for ordering to make things easy. 
x         = np.zeros(blocks, dtype=float)  # Target calculated for each height. x = 1/difficulty
num_parents=np.zeros(blocks, dtype=float)  # Number of parents a block has.
not_a_common_ancestor = np.zeros(blocks, dtype=bool) # used to identify consensus blocks.
latency   = np.full(blocks, latency_,  dtype=float)  # fill blocks with base latency they have in sending & recieving blocks
hashrate  = np.full(blocks, hashrate_, dtype=float) # fill blocks with base hashrate

# parents[of h][are these h's]  This will have sublists with varying # of elements in sublist
# I should change this to an np array for better speed but i'll still need to do slow resizing.
parents = [[] for _ in range(blocks)]

# pre-populate solvetime list so that we can get solvetime[h] *= L[h]/x[h] during runs. 
solvetime = [-math.log(ra.random()) for _ in range(blocks)] 

# randomly choose & change the populated latencies for the fraction of blocks ('miners') who have latency_2
for h in range(Nb+20,blocks):
    if ra.random() < hrf_latency_2: latency[h] *= latency_2

attack_on = 0
if use_attack:
    for h in range(Nb+20,blocks):
        if h > attack_length and h < attack_length*2: hashrate[h] *= attack_size
        '''
        if attack_on: hashrate[h] *= attack_size
        if h > attack_length and h%attack_length == 0:
            if attack_on == 0:  attack_on = 1
            else:  attack_on = 0
        '''
                
latency_change_on = 0               
if use_latency_change:
    for h in range(Nb+20,blocks):
        if h > 3*latency_change_length and h < 4*latency_change_length: 
            latency[h] *= latency_change_size 
        '''
        if latency_change_on: latency[h] *= latency_change_size
        if h > latency_change_length and h%latency_change_length == 0:
            if latency_change_on == 0: latency_change_on = 1
            else:  latency_change_on = 0   
        '''

# Genesis block
x[0] = 1 ; Nb_Nc[0] = Nb_Nc_target ; num_parents[0] = 0 ; parents[0].append(0)
solvetime[0] *= 1/hashrate_/x[0]
time[0] = solvetime[0]

# initialize 1st difficulty window plus a 20-block buffer for siblings
if DAA == 2: buffer = n2
else: buffer = 20
for h in range(1, Nb+buffer): 
    x[h] = .47/latency_/hashrate_ ; Nb_Nc[h] = Nb_Nc_target ; num_parents[h] = 1 ; parents[h].append(h-1)
    solvetime[h] *= 1/hashrate_/x[h-1]
    time[h] = solvetime[h] + time[h-1] 
    
##### START MINING #######

for h in range(Nb+buffer,blocks): 

	# The following solvetime is cheat because it uses x[h-1] instead of x[h] to prevent complexity. 
	# We don't know x[h] unless we know parents, but parents change as solvetime increases.
	# The solution would be to simulate granular hashing like real hashing. This isn't bad for large n.
    # A real chain could use a hash-ordered parent for its x which wouldn't be as good as this.
    solvetime[h] *= 1/hashrate[h]/x[h-1]
    time[h] = solvetime[h] + time[h-1]
	
    # For this block, find non-Nc blocks (siblings) and parents.
    seen.fill(0)        # height is the index. 1 as the stored value indicates a seen block that can't be a parent
    Nb_blocks.fill(0)   # Number of blocks up to Nb into the past is the index. Value is a height.
    i = 0 # keeps track of how many Nb blocks have been found (non-siblings)
    k = 1 # keeps track of how many blocks into the past we're looking (ordered by the cheat of using real time timestamps)
    while i < Nb: # Nb ("look-back" blocks) has been changed to 8*parents_desired if using parent DAA
        if not use_exact_latency: 
            latency[h] = max(0,(ra.random()*2*latency[h] + ra.uniform(-a_var,a_var))) # 2x bc user defines latency as a median.
        # Current block cannot yet see prior block if either latency of sending by prior block or receiving 
        # by current block is longer than the solvetime between them.
        if max(latency[h],latency[h-k]) > time[h] - time[h-k]:   # Can't see block h-k due to latency
            # Remember non-Nc blocks. A cheat to determine Nc blocks using our global knowledge of time, order, & latency.
            # This can't be a consensus block because a descendant will see them both as a parent.
            not_a_common_ancestor[h-k] = 1
            not_a_common_ancestor[h] = 1
        else:
            # Combined with the above 'if', determine if previous block was an Nc consensus block.
            # This occurs when latency before & after h-k-1 block was less than the before & after solvetimes.
            # In other words, there are no siblings as a result of latency before or after h-k-1. 
            # This simulation assumes honest miners who don't ignore any parent they saw.
            if not_a_common_ancestor[h-k-1] == 0: 
                Nc_blocks[h-k-1] = 1 # 1 = previous block was an Nc block
            Nb_blocks[i] = h-k
            # Store parents of the current i of Nb blocks in the past as seen[].
            # If seen, they aren't a parent of this block at h. The remaining Nb blocks are parents.
            for j in range(len(parents[h-k])):
                seen[parents[h-k][j]] = 1  # seen[height] = 1 means it not be a parent
            i += 1 # increment until we've looked back i = Nb blocks
        k += 1 # keeps track of actual blocks in past which is > i due to siblings.
    '''
We've finished looking into the past Nb ancestors for this block at h. In practice, a scheme using hash-ordering would make sure all validators see the same Nb blocks. This simulator cheated by using knowledge of real time to pick them. Now find which of the Nb blocks are parents of this block. A parent has to be in Nb_blocks[] and not in seen[]. This assumes no parents are older than Nb. Nb is therefore a rigid cut-off for valid parents and a consensus confirmation range. A parent beyond that range should be orphaned by everyone, but if not, a fork results & PoW will pick the winning fork. 
    '''
    for j in range(Nb): 
        if seen[Nb_blocks[j]] == 0: 
            parents[h].append(Nb_blocks[j]) # this is a parent bc no other potential parents saw it.
    num_parents[h] = len(parents[h])

    sum_x_inv = 0

# Using Nb/Nc DAA. 
    if DAA == 0:
        Nc=0 
        for m in range(Nb):
            sum_x_inv += 1/x[Nb_blocks[m]]
            Nc += Nc_blocks[Nb_blocks[m]]
            
        x1 = Nb/sum_x_inv  # harmonic mean
        Nc = max(0.1, Nc)  # prevent divide by zero
        Nb_Nc[h] = Nb/(Nc+Nc_adjust)

        x[h] = x1*(1 + (Nb_Nc_target - Nb_Nc[h])/n0)
        
        # CF = Q/max(0.1,lambertw(complex(max(.1,Nb_Nc[h]-1)),k=0).real)   
        # x[h] = x1*(1 + (CF - 1)/n0)        
        
# Using parents DAA.
    elif DAA == 1:
        # calculate harmonic mean of parents' targets
        # print (h,num_parents[h])
        for m in range(int(num_parents[h])):
            #print(h,m, x[parents[h][m]])
            sum_x_inv += 1/x[parents[h][m]]
        #if h == 100: sys.exit()
        x1 = num_parents[h]/sum_x_inv
        x[h] = x1*(1 + (parents_desired - num_parents[h])/n1)
        
# &Using SMA DAA            
    elif DAA == 2:
        # calculate harmonic mean of parents' targets
        for m in range(n2):
            sum_x_inv += 1/x[h-m-1]
        x1 = n2/sum_x_inv
        x[h] = x1*(time[h]-time[h-n2])/sma_block_time/n2
        # print(attack_on, h, x[h], parents[h])
# end generating blocks

print("             Mean  StdDev")
print("x:           {:.3f} {:.3f}".format(st.harmonic_mean(x), st.pstdev(x)))
print("solvetime:   {:.3f} {:.3f}".format(st.mean(solvetime), st.pstdev(solvetime)) )# 
print("Nb/Nc:       {:.3f} {:.3f} ".format(blocks/sum(Nc_blocks), st.pstdev(Nb_Nc)))
print("num_parents: {:.3f} {:.3f}".format(st.mean(num_parents), st.pstdev(num_parents)))
print("Time per Nc block per latency: {:.3f}".format(time[blocks-1]/(sum(Nc_blocks))/st.mean(latency)))

print(f"\nRun time: {timer.time() - start:.2f} seconds")


height = np.arange(0,blocks,1)
DAG_width = [0]*(blocks)
mean_length = 300
for i in range(blocks):
    if i > mean_length + 1 : DAG_width[i] = st.mean(num_parents[i-mean_length:i])

Nc_Nb_mean = [0]*(blocks)

if not DAA ==2:
    consensus_time_mean = [0]*(blocks)
    for i in range(blocks):
        if i > mean_length + 1 : 
            consensus_time_mean[i] = (time[i] - time[i-mean_length])/sum(Nc_blocks[i-mean_length:i])/10
        
#E difficulty = 1 / x

# Create a line plot
plt.plot(height, x, label='target x')
plt.plot(height, latency, label='latency')
plt.plot(height, hashrate, label='hashrate')
plt.plot(height, DAG_width, label='DAG width, '+str(mean_length) +' blocks')
if not DAA ==2:
    plt.plot(height, consensus_time_mean, label='consensus time/10, '+str(mean_length) +' blocks')
# plt.plot(height, num_parents, label='num parents')

# Add title and labels
if DAA == 0: title = "Nb/Nc DAA, Nb = {:.0f}, n = {:.0f}:".format(Nb,n0)
if DAA == 1: title = "Parent DAA, parents = {:.2f}, n = {:.0f}".format(parents_desired, n1)
if DAA == 2: title = "SMA DAA, block_time = {:.2f}, n = {:.0f}".format(sma_block_time, n2)  

plt.title(title)
plt.xlabel('height')
plt.ylabel('target, hashrate, DAG width. Consensus time in 10x units of latency')


# Add a legend
plt.legend()

# Display the plot
plt.show()

'''
This simulates two PoW DAA's for Bob McElrath's Braidpool DAG. These DAA's target a DAG width without using timestamps or estimating hashrate or latency. This is possible because increasing hashrate or latency causes DAG width to increase to the same extent when the DAG is made as fast as possible. Increasing difficulty has the same effect on both and makes it narrow again. In setting target * hashrate * latency, time cancels. Consensus on Braidpool's scheme is achieved when the DAG width is 1, i.e. all parents and children of a certain "generation" of blocks see only 1 block in that generation which makes it easy to identify and have consensus for everyone to know the current state. This 1-block width occurs only if the difficulty is easy enough for the current hashrate so that there have not been any blocks mined before or after it within 1 latency period so that all honest parent and child blocks have time to see it. But for this 1-block consensus to occur as fast as possible, the difficulty also needs to be as easy as possible. Bob's Braidpool article discusses the math to try to identify the optimum solve rate to balance these two opposing effects to find the optimum difficulty: 
https://github.com/braidpool/braidpool/blob/main/docs/braid_consensus.md 

DAA targeting a number of parents:
-------------------------------
x = x1*(1 + (parents_desired - parents_observed)/n)
x1 = harmonic mean of the parent's targets
When parents_desired = 1.44, it is close to Braidpool's ideal Nb/Nc = 2.42

DAA using targeting Nb/Nc:
---------------
x = x1*(1 + (Nb_Nc_desired - Nb_Nc_measured)/n)
Nb_Nc_desired = 2.42 (via Braidpool, fastest consensus)
x1 = harmonic mean of past Nb blocks' targets. 
This algo looks back either Nb or Nc blocks to measure the other (Nc or Nb). When looking back by a fixed number of Nb blocks: 
Nb_Nc_measured = Nb_setting/(Nc_measured+1.5)
The +1.5 was determined by experiment to more quickly and accurately give 2.42 results when the setting is 2.42, without regard to the Nb setting or filter.

Deprecated DAA calculation for Nb/Nc due to having more variation in x

        if use_approx_DAA:
            if Nb_Nc[h] < 1.5:  CF = 2
            elif Nb_Nc[h] > 3.1:   CF = 0.66**(Nb_Nc[h] /5)
            else:               
                CF = Nb_Nc_target/(Nb_Nc[h])        
        else: CF = Q/max(0.1,lambertw(complex(max(.1,Nb_Nc[h]-1)),k=0).real)   

        x[h] = x1*(1 + (CF - 1)/n)  

Similarly, medthods based on the probability of seeing different numbers of parents in the parent method did not work as well. 
'''
