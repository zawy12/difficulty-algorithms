import statistics as st
import math
import random as ra
# Copyright 2025 zawy, MIT license

# I'm editing today without testing. See previous version if this does work.

# Usage: Run on command line to see the DAG's difficulty algorithm (DAA) output. Change variables below for testing.
# Detailed explaination is below.

blocks      = 5000	# number of  blocks in this simulation
a           = 1         # a = latency. Express 'a' as a MEDIAN and MEAN latency. Simulator randomnly varies from 0 to 2 * a 
a_var       = 0         #  a_var allows additional random variation
use_exact_latency = 1   # select this = 1  to make latency for every peer exactly 'a' instead of a median.
L           = 1         # L = lambda = hashrate. L_var allows additional random variation.
n           = 100	    # mean lifetime of DAA filter to smooth variations and slow it down. 100 is good.
use_parents = 1         # tells program to use parents DAA instead of Nb/Nc DAA
desired_parents = 2     # 2 for this value is optimal
Nb          = 10        # Blocks for Nb/Nc DAA to look into the past to count Nc blocks  
Q           = 0.703467  # Bob's theoretical optimal setting of Q = a*x*L to get fastest blocks. This value is solution to Q=e^(-Q/2). 
Nb_Nc_target = (1+1/Q)  # This is 2.42 from the above Q, but now 2.71828 appears better.

'''
This simulates two PoW DAA's for Bob McElrath's Braidpool DAG. They target a DAG width without using timestamps or
estimating hashrate or latency. This is possible because increasing hashrate or increasing latency cause DAG
width to increase. Increasing difficulty can make it narrow again. A hashrate * latency factor cancels time units in both.
When a narrow DAG is operating at the fastest speed to get consensus, changing the difficulty becomes very effective
at changing the DAG width to keep it achieving consensus at the fastest speed possible. Bob's Braidpool article discusses 
the math. Consensus is achieved when the DAG width is 1, i.e. all parents and children of a certain "generation" of blocks
see only 1 block in the middle and it is therefore easy to have consensus and everyone know the current state. This 1-block
width occurs only if the hashrate for the current difficulty setting is slow that there have not been any blocks mined before
or after that block for 1 latency period before and 1 period after it. But for this 1-block consensus to occur as fast as possible,
the block solves must be comeing as fast as possible. Bob's Braidpool article discusses the math to try to identify the optimum 
solve rate (i.e. hashrate * target) to balance these two opposing effects:
https://github.com/braidpool/braidpool/blob/main/docs/braid_consensus.md

It turns out the fastest consensus occurs when Bob's Nb/Nc metric for DAG width = 2.71828, 10% more than what his theory predicted.  
While running this simulator to try to test his math, I realized there could be a much easier metric to measure which is the
average number of parents. It turns out a mean of 2.000 parents gives the fastest consensus, and the StdDev of parents is
1.0000. This results in Bob's Nb/Nc ratio being 2.71828 and the average time between consensus is 2.71828 times the latency.
To be clear, to set a target (2^256 / difficulty) for a particular block being mined, the consensus rule is that he and future
validators only count his parents (which will typically be from 1 to 4) and get the harmonic mean of their targets and then use
the equation below. This is incredibly simple compared to other DAGs and it may be the fastest possible consensus.

DAA using the number of parents:
-------------------------------
x = x1*(1 + 2/n - parents_observed/n)
x1 = harmonic mean of the parent's targets

DAA using Nb/Nc:
---------------
x = x1*(1 + 2.718/n - Nb/Nc/n)
x1 = harmonic mean of past Nb blocks' targets
    
'''
if use_exact_latency: print("\nUser choose to use 'a' as the exact latency between every peer.\n")

if use_parents ==1:
    print("=====   User selected parent-count DAA. =====")
    print("x = x_parents_hmean * ( 1 + desired_parents/n - num_parents_measured\n")
    print("median a\tL\tparents desired\t\tn\t\tblocks")
    print("{:.2f}\t\t{:.1f}\t\t{:.0f}\t\t\t\t{:.0f}\t\t{:.0f}".format(a, L, desired_parents, n, blocks))
else: 
    print("=====   User selected Nb/Nc ratio DAA. Q=0.7035 =====")
    print("x = x_Nb_hmean * ( 1 + (1+1/Q)/n - Nb/Nc/n")
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
solvetime[0] = -1/x[0] * math.log(ra.random()) * L + ra.uniform(-1.0*L_var,L_var))
time[0] = solvetime[0]

# initialize 1st difficulty window
for h in range(1,Nb+20): # plus 20 because delay prevents some blocks from deing seen
    x[h] = 1 ; Nb_Nc[h] = Nb_Nc_target ; num_parents[h] = 1 ; parents[h].append(h-1)
    solvetime[h] = -1/x[h-1] * math.log(ra.random()) * L
    time[h] = solvetime[h] + time[h-1] 
    
##### START MINING #######
for h in range(Nb+20,blocks):
    # if h == 500: L *= 20.0  # for testing sudden hashrate change
    num_parents[h]=0
	# The following solvetime cheat using x[h-1] instead of x[h] prevents complexity. 
	# We don't know x[h] unless we know parents. But parents change as solvetime
	# increases. The solution would be to calculate delays, find parents, get x[h], 
	# hash a little which gets closer to the delays, find parents again, ... repeat.
    solvetime[h] = -1.0/x[h-1] * math.log(ra.random()) * L
    time[h] = solvetime[h] + time[h-1]   	
	
# for this block, find non-Nc blocks (siblings), parents, and avoid incest
    seen = [0]*(blocks)
    Nb_blocks = []
    k = 0 
    for i in range(1,Nb + 20): # must be less than 20 blocks within 1 latency
        next_i = 0
        latency = (ra.random()*2*a + ra.uniform(-a_var,a_var)) # 2x assumes user assigned a = median.
        if use_exact_latency ==1 : latency = a
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

# use parents = 2 method to calculate x instead of Nb/Nb = 2.42 method.
    if use_parents == 1:
        sum_x_inv = 0
        for i in range(len(parents[h])):
            sum_x_inv += 1/x[parents[h][i]]
        x1 = len(parents[h])/sum_x_inv
        x[h] = x1*(1 + desired_parents/n - len(parents[h])/n)

# Using Nb/Nc method. Get x1 (harmonic mean of targets), Nc, and Nb_Nc for difficulty calculation
    else:
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
                Nb_Nc[h] = Nb_temp/Nc
    
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
print("Nb/Nc:       {:.3f} {:.3f} ".format(blocks/sum(Nc_blocks), st.pstdev(Nb_Nc)))
print("num_parents: {:.3f} {:.3f}".format(st.mean(num_parents), st.pstdev(num_parents)))
print("Nc blocks: " + str(blocks - sum(not_a_common_ancestor)))
print("Time/Nc/a: {:.3f} (The speed of consensus as a multiple of latency.)".format(time[blocks-1]/(sum(Nc_blocks))/a))
