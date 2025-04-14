# DAG difficulty algorithm simulator.

# Copyright 2025 zawy, MIT license

# This command-line script tests 3 different difficulty adjustment algorithms (DAA's) for a DAG PoW. 
# Two of them are remarkable in not needing timestamps or measurements of hashrate or latency. 

# Comments are extensive and instructive. A discussion is at the end of this file.

# Usage: Run on command line to see the output. Change variables below for testing. 

import time as timer
import statistics as st
import math
import random as ra
import sys
import numpy as np # on windows i had to use "py -m pip install numpy"
try: import matplotlib.pyplot as plt
except ImportError: print("Install matplotlib to see plots.")
try: import networkx as nx
except ImportError: print("Install networkx & pygraphviz to see DAG graph.")

#  Nb blocks are the number of blocks to look back in the past to find parents and Nc blocks. 
#  Nc blocks are 1-block cohorts, i.e. all ancestors & descendants are connected through them.
DAA_0           = 0         # = 1 do Nb/Nc DAA
DAA_1           = 1         # = 1 do Parent DAA
DAA_2           = 0         # = 1 do SMA DAA
show_single_plots = 1       # = 1 to plot results
show_combined_plots  = 0    # = 1 to plot target and parent results for DAAs together

show_dag        = 0         # = 1 to show a sample of the DAG graph using the next two settings
start_dag_height = 10000
dag_blocks       = 50       # the DAG graph may not be very visible if this is > 50 blocks   

blocks          = 13000	    # number of  blocks in this simulation
base_latency    = .67      # base latency. Enter as a MEAN latency. Use = 1 for time to be in units of latency. 
base_hashrate   = .66         # base hashrate, lambda. Resulting difficulty = 2.2 * base_latency * hashrate (parents = 1.44)
use_exact_latency = 1   # '1' to make latencies exactly for all nodes instead of random from 0 to 2x base_latency
latency_var = 0         # allows an additional random variation in latency.

# "Network Topology" checks to see what happens when a portion of miners have higher latency.
hrf_latency  = 0.15     # Hash Rate Fraction that has 2nd latency level. Can be steps incrof increasing latency or constant

use_hrf_latency_steps = 0       # = 1 for HRF to have a different latency 
latency_hrf_step_start = 0.05
latency_hrf_step_size = 0.05    # plots are better if this is positive
latency_hrf_step_blocks = 500   # how long a step of latency is applied before next step
if use_hrf_latency_steps: base_hashrate = 1/4/base_latency # this hopefully allows difficulty to be on a scale that plots better

use_hrf_latency = 0         # HRF steps above must be disabled for a constant 2nd level of latency to apply to HRF
latency_hrf_multiple = 2    # 2nd latency level as mutiple of base_latency 

latency_hrf_plot = [0]*(blocks) # stores HRF's latency for plotting so that we can see the effect

n_0          = 60       # Filter for Nb/Nc DAA. Mean lifetime of DAA filter to smooth variations and slow it down. 
n_1          = 200      # Filter for Parent DAA, needs to be Nb*n/16 for equal stability to Nb/Nc DAA
n_2          = 190      # Filter for SMA. This is about 2.5x the n value for the parent method to get equal stability.
block_time_desired  = 1.53  # SMA block time desired. Use 1.53 if latency and latency = 0.67. 1.53/0.67 = 2.3 important?
parents_desired     = 1.44  # 1.44 gives Nb/Nc =~ 2.42, the ideal.
Nb_0         = 20       # Blocks for Nb/Nc to look into past. Smaller is faster but can miss parents & not see Nc blocks if too small. Needs to be 20 for constant hashrate to almost always see at least 1 Nc.
Nb_1  = int((parents_desired+0.5) * 4)    # >7 blocks solved in 1 latency-adjusted 'block time' will miss a parent 
Nb_2  = int(10*base_latency/block_time_desired + 0.5)  # >7 blocks solved in 1 latency-adjusted block-time will miss a parent  
Nc_adjust       = 1.35   # To make Nb/Nc DAA more accurate, this needs to be about 1.35. Works for all settings.
Q               = 0.703467  # 0.703467 is optimal Q = a*x*L to get fastest 1-block cohort. This value solves Q=e^(-Q/2).  
Nb_Nc_desired    = (1+1/Q)  # For Nb/Nc DAA, this is our target Nb/Nc = 2.4215 from the above Q. 

use_attack          = 0     # if = 1, use on-off mining attack 
attack_length       = 4000  # number of blocks for on-off cycle if latency changes are not also selected
attack_size         = 2     # multiplier for the increase in hashrate. Nb selection below assumes > 1.
    
use_latency_change     = 0     # if = 1, a latency "attack". Changes base_latency before hHRF if HRF is applied.
latency_change_length  = 4000  # number of blocks for on-off cycles if mining attack above is not also selected
latency_change_size    = 2     # multiplier for the increase in latency. Nb selection below assumes > 1.

initial_target = .45/base_latency/base_hashrate # for initializing blocks before mining
buffer = 20     # needed for initiliazing the chain for hard-to-explain reasons

# Pre-populate solvetime list so that we can compare DAAs better. Later we'll use solvetime[h] *= 1/hashrate[h]/x[h] during runs.
solvetime_all = [-math.log(ra.random()) for _ in range(blocks)]
latency     = np.full(blocks, base_latency,  dtype=float) # fill blocks with base latency in sending & recieving blocks
hashrate    = np.full(blocks, base_hashrate, dtype=float) # fill blocks with base hashrate

DAA = 0

# The following cheat catches all parents so that plots don't lie about chain width. In an actual chain, Nb must have some limit and will cause forks if a parent falls outside the range, and PoW will pick the winner. It can be considered a weak confirmation of consensus. Larger Nb slows simulation down.
if use_latency_change: 
    Nb_1 *= latency_change_size
    Nb_2 *= latency_change_size

# if needed, print notifications that hashrate and latency will be changed
if use_exact_latency: 
    print("User chose to use exact mean latencies instead of randomly varying around it.")
if use_attack and use_latency_change:
    print("User selected a {:.2f}x increase in hashrate and {:.2f}x increase in latency.".format(attack_size,        latency_change_size))
elif use_attack: 
    print("User selected an on-off attack.")
    print("Attack_size = {:.2f}, Attack_length = {:.2f}".format(attack_size, attack_length))
elif use_latency_change:
    print("User selected changes in latency.")
    print("Latency change size = {:.2f}, Latency change length = {:.2f} blocks".format(latency_change_size, latency_change_length))

# pre-populate hashrate and latency changes for all DAA runs

attack_on = 0 ; latency_change_on = 0 
for h in range(blocks):
    # If both on-off mining attack & latency changes are selected, just show 1 cycle of each based on total blocks        
    if use_latency_change and use_attack:
        if h > blocks/5 and h < 2*blocks/5: hashrate[h] *= attack_size
        if h > 3*blocks/5 and h < 4*blocks/5: latency[h] *= latency_change_size       

    # Adjustments for a cyclic attacks    
    elif use_attack:
        if attack_on: hashrate[h] *= attack_size
        if h > attack_length and h%attack_length == 0:
            if attack_on == 0:  attack_on = 1
            else:  attack_on = 0
            
    # Adjustments for cyclic latency changes
    elif use_latency_change:
        if latency_change_on: latency[h] *= latency_change_size
        if h > latency_change_length and h%latency_change_length == 0:
            if latency_change_on == 0: latency_change_on = 1
            else:  latency_change_on = 0 
    if use_hrf_latency_steps:
        if h%latency_hrf_step_blocks == 0: latency_hrf_step_start += latency_hrf_step_size
        latency_hrf_plot[h] = base_latency + latency_hrf_step_start 
        if ra.random() < hrf_latency: latency[h] = base_latency + latency_hrf_step_start
    elif use_hrf_latency:
        # randomly change the populated latencies for the fraction of blocks ('miners') who have latency_2
        if ra.random() < hrf_latency: latency[h] *= latency_hrf_multiple # latency[h] was already pre-loaded with latency_
    if not use_exact_latency: # make latency vary randomly from 0 to 2x with possible additional variation. 
        latency[h] = max(0,(ra.random()*2*latency[h] + ra.uniform(-latency_var,latency_var))) 

# initialize arrays
def initialize_global_arrays(DAA_, Nb_):
    global Nc_blocks, Nb_Nc, time, x, d, num_parents, solvetime, Nb, DAA, parents, children
    Nc_blocks = np.zeros(blocks, dtype=bool)   # index is height. True = a consensus block. 
    Nb_Nc     = np.zeros(blocks, dtype=float)  # The ratio Nb/Nc as observed by the block at height h looking back Nb
    time      = np.zeros(blocks, dtype=float)  # Simulator has access to precise time for ordering to make things easy. 
    x         = np.full(blocks, 1, dtype=float)  # Target calculated for each height. x = 1/difficulty
    d         = np.full(blocks, 1, dtype=float)
    num_parents=np.zeros(blocks, dtype=float)  # Number of parents a block has.
    solvetime = np.zeros(blocks, dtype=float)
    Nb        = Nb_    
    DAA       = DAA_

    # parents[of h][are these h's]  This sublist will have varying # of elements.
    parents = [[] for _ in range(blocks)]
    children= [frozenset() for _ in range(blocks)]
       
def print_table(data):   
    col_widths = [max(len(str(row[i])) for row in data) for i in range(len(data[0]))]
    for i, col in enumerate(data[0]): print(f"{col:<{col_widths[i]}}", end="  ")
    # print("\n" + "-" * (sum(col_widths) + 3 * (len(col_widths) - 1)))
    print()
    for row in data[1:]:
        for i, value in enumerate(row): print(f"{str(value):<{col_widths[i]}}", end="  ")
        print()

def print_messages():
        
    if DAA == 0: 
        print("=====   Nb/Nc ratio DAA  =====")
        print("x = x_harmonic_mean_of_Nb * ( 1 + (Nb_Nc_desired - Nb_Nc_observed)/n)\n")
        Nb_Nc_temp = int(Nb_Nc_desired*10000)/10000
        data = [['latency','hashrate','Nb','Nb/Nc desired','filter n', 'blocks'],
            [base_latency,base_hashrate,Nb,Nb_Nc_temp,n_0,blocks]]
    elif DAA == 1: 
        print("=====   Parent-count DAA  =====")
        print("x = x_harmonic_mean_of_parents * ( 1 + (parents_desired - parents_measured)/n)\n")
        data = [['latency','hashrate','parents desired','filter n', 'blocks'],
            [base_latency,base_hashrate,parents_desired,n_1, blocks]]
    elif DAA == 2: 
        print("=====   SMA DAA  =====")
        print("x = x_harmonic_mean_of_n * timespan_observed / timespan_desired\n")
        data = [['latency','hashrate', 'block_time','n', 'blocks'],
            [base_latency,base_hashrate,block_time_desired,n_2,blocks]]  

    print_table(data)
    
def do_mining():
    Nb_blocks       = np.zeros(Nb,     dtype=np.int32) # Array of size Nb. Will store heights
    seen            = np.zeros(blocks, dtype=bool) # index is height. True = previously 'seen' = it's not a parent block.
    had_a_sibling   = np.zeros(blocks, dtype=bool) # if it had a sibling within latency before or after, it's not an Nc.
    
# Genesis block
    x[0] = initial_target ; Nb_Nc[0] = Nb_Nc_desired ; num_parents[0] = 0
    solvetime[0] = solvetime_all[0]/base_hashrate/x[0]
    time[0] = solvetime[0]
    d[0]=1/x[0]
    
# Initialize 1st difficulty window plus a 20-block buffer for siblings
    for h in range(1, Nb+buffer): 
        x[h] = initial_target; Nb_Nc[h] = Nb_Nc_desired ; num_parents[h] = 1 ; parents[h].append(h-1)
        solvetime[h] = solvetime_all[h]/base_hashrate/x[h-1]
        time[h] = solvetime[h] + time[h-1] 
        Nc_blocks[h] = int(2*(1+1/Nb_Nc_desired)*ra.random()) # trying to estimate
        d[h]=1/x[h]

# Start Mining
    for h in range(Nb+buffer,blocks): 
        if h-Nb-buffer == 3000: 
            print("=== Remaining time: " + str(int((timer.time()-start)*(blocks-Nb-buffer-3000)/3000)) + " seconds ===")
	# The following solvetime is cheat because it uses x[h-1] instead of x[h] to prevent complexity. 
	# We don't know x[h] unless we know parents, but parents change as solvetime increases.
	# The solution would be to simulate granular hashing like real hashing. This isn't bad for large n.
    # A real chain could use a hash-ordered parent for its x which wouldn't be as good as this.
        solvetime[h] = solvetime_all[h]/hashrate[h]/x[h-1]
        time[h] = solvetime[h] + time[h-1]
	
        # For this block, find non-Nc blocks (siblings) and parents.
        seen.fill(0)        # height is the index. 1 as the stored value indicates a seen block that can't be a parent
        Nb_blocks.fill(0)   # Number of blocks up to Nb into the past is the index. Value is a height.
        i = 0 # keeps track of how many Nb blocks have been found (non-siblings)
        k = 1 # keeps track of how many blocks into the past we're looking
        while i < Nb: # Nb ("look-back" blocks) has been changed to 8*parents_desired if using parent DAA
        # Current block cannot yet see prior block if either latency of sending by prior block or receiving 
        # by current block is longer than the solvetime between them. the max() causes a harsh latency and 
        # complicates comparisons when exact latency option isn't used.
            if max(latency[h],latency[h-k]) > time[h] - time[h-k]:   # Can't see block h-k due to latency
            # Remember non-Nc blocks. A cheat to determine Nc blocks using our global knowledge of time, order, & latency.
            # This can't be a consensus block because a descendant will see them both as a parent.
                had_a_sibling[h-k] = 1
                had_a_sibling[h] = 1
            else:
            # Combined with the above 'if', determine if previous block was an Nc consensus block.
            # This occurs when latency before & after h-k-1 block was less than the before & after solvetimes.
            # In other words, there are no siblings as a result of latency before or after h-k-1. 
            # This simulation assumes honest miners who don't ignore any parent they saw.
                if had_a_sibling[h-k-1] == 0: 
                    Nc_blocks[h-k-1] = 1 # 1 = block before h-k was an Nc block
                Nb_blocks[i] = h-k
            # Store parents of the current i of Nb blocks in the past as seen[].
            # If seen, they aren't a parent of this block at h. The remaining Nb blocks are parents.
                for j in range(len(parents[h-k])):
                    seen[parents[h-k][j]] = 1  # seen[height] = 1 means it isn't a parent
                i += 1 # increment until we've looked back i = Nb blocks + blocks not seen by latency
            k += 1 # keeps track of actual blocks in past which is > i due to siblings.
        '''
We've finished looking into the past Nb ancestors for block h. In practice, a scheme using hash-ordering would ensure all validators see the same Nb blocks. This simulator cheated by using knowledge of real time to pick them. Now find the Nb blocks that are parents of this block h. A parent has to be in Nb_blocks[] and not in seen[]. This assumes no parents are older than Nb. Nb is therefore a rigid cut-off for valid parents and a consensus confirmation range. If parent beyond that range is acknowledged by other blocks, it causes a fork & PoW will determine the winning fork, orphaning the other. 
        '''
        for j in range(Nb): 
            if seen[Nb_blocks[j]] == 0: 
                parents[h].append(Nb_blocks[j]) # this is a parent bc no other potential parents saw it.
        num_parents[h] = len(parents[h])

        sum_x_inv = 0
# Using Nb/Nc DAA.
        if DAA == 0:
            Nc = 0 
            for m in range(Nb):
                sum_x_inv += 1/x[Nb_blocks[m]]
                Nc += Nc_blocks[Nb_blocks[m]]  
            x1 = Nb/sum_x_inv  # harmonic mean
            Nb_Nc[h] = Nb/(Nc+Nc_adjust)
            x[h] = x1*(1 + (Nb_Nc_desired - Nb_Nc[h])/n_0)
            # CF = Q/max(0.1,lambertw(complex(max(.1,Nb_Nc[h]-1)),k=0).real)   
            # x[h] = x1*(1 + (CF - 1)/n0)        
# Using Parents DAA.
        elif DAA == 1:
            # calculate harmonic mean of parents' targets
            # harmonic mean of targets is same as the inverse of mean of difficulties
            for m in range(int(num_parents[h])):
                sum_x_inv += 1/x[parents[h][m]]
            x1 = num_parents[h]/sum_x_inv
            x[h] = x1*(1 + (parents_desired - num_parents[h])/n_1)           
# Using SMA DAA            
        elif DAA == 2:
            if h < n_2:
                for m in range(h):
                    sum_x_inv += 1/x[h-m-1]
                x1 = 1/sum_x_inv  # h divides out in next line
                x[h] = x1*(time[h]-time[0])/block_time_desired
            else: 
                for m in range(Nb): # avoids averaging sibling blocks
                    sum_x_inv += 1/x[Nb_blocks[m]] 
                    # sum_D += d[Nb_blocks[m]]
                for m in range(n_2-Nb): # get the rest of n2. need to check for off-by-1
                    sum_x_inv += 1/x[Nb_blocks[Nb-1]-m-1] # m is initially 0. Starts at 1 block before oldest Nb
                    # sum_D += d[Nb_blocks[Nb-1]-m-1]                
                x1 = 1/sum_x_inv # n2 divides out in next line
                # if topology is uneven, a sibling could be older than newest Nb so timespan not quite right 
                # x1 = 1/sum_D
                x[h] =  x1*(time[Nb_blocks[0]] - time[Nb_blocks[0]-n_2])/block_time_desired
        d[h] = 1/x[h]

def print_finish_message():
    global SD_x ; SD_x = st.pstdev(x)
    global SD_parents ; SD_parents = st.pstdev(num_parents)
    global SD_d ; SD_d = st.pstdev(d)
    global mean_parents; mean_parents = st.mean(num_parents)
    global mean_d; mean_d = st.mean(d)
    global mean_st; mean_st = st.mean(solvetime)
    global mean_Nb_Nc; mean_Nb_Nc = blocks/sum(Nc_blocks)
    global Nc_time; Nc_time = time[blocks-1]/(sum(Nc_blocks))/st.mean(latency)
    
    print(f"=== Total Time: {finish_time:.2f} seconds, {finish_time*1000/(blocks-Nb-buffer):.4f} seconds/1000 blocks.\n")
    print("             Mean  StdDev")
    print("x:           {:.3f} {:.3f}".format(st.harmonic_mean(x), SD_x))
    print("d:           {:.3f} {:.3f}".format(mean_d, SD_d))
    print("solvetime:   {:.3f}".format(mean_st))
    print("Nb/Nc:       {:.3f}".format(mean_Nb_Nc))
    print("num_parents: {:.3f} {:.3f}".format(mean_parents, SD_parents))
    print("Time per Nc block per mean latency: {:.3f}\n".format(Nc_time))

def single_plots():
    height = np.arange(0,blocks,1)
    mean_length = 200 # smooth out the variables we're interested in
    
    DAG_width = [st.mean(num_parents)]*(blocks)
    for i in range(100,blocks):
        n = min(i,mean_length)
        DAG_width[i] = st.mean(num_parents[i-n:i])
        
    consensus_time_mean = [0.5]*(blocks)
    for i in range(100,blocks):
        n = min(i,mean_length)
        consensus_time_mean[i] = (time[i] - time[i-n])/max(1,sum(Nc_blocks[i-n:i]))/10

    plt.plot(height, d, label='difficulty',  linewidth=2)
    # plt.plot(height, x, label='target',  linewidth=2)
    plt.plot(height, DAG_width, label='Parents, ' + str(mean_length) + '-block mean', linewidth=2)

    if use_hrf_latency_steps:
        plt.plot(height, latency_hrf_plot, label='latency of hashrate fraction', linewidth=2)
    else: 
        plt.plot(height, latency, label='latency')
        plt.plot(height, hashrate, label='hashrate', linewidth=2)
    plt.plot(height, consensus_time_mean, label='(time per Nc)/10, '+ str(mean_length) +'-block mean', linewidth=2)
 
    if DAA == 0: 
        title = "Nb/Nc DAA\nNb/Nc desired = {:.2f}, Blocks to look-back for parents and Nc blocks = {:.0f}, smoothing filter n = {:.0f}:".format(Nb_Nc_desired, Nb_0,n_0, Nc_time)
    if DAA == 1: 
        title = "Parent DAA\nParents desired = {:.2f}, Blocks to look-back for parents = {:.0f}, smoothing filter n = {:.0f}".format(parents_desired, Nb_1, n_1, Nc_time)
    if DAA == 2: 
        title = "SMA DAA\nBlock time desired = {:.2f}, Blocks to look-back for parents = {:.0f}, smoothing filter n = {:.0f}".format(block_time_desired, Nb_2, n_2, Nc_time) 

    if use_hrf_latency_steps: title += "\nHashrate fraction with plotted latency: = {:.2f}".format(hrf_latency)

    title += "\nMeans (SDs): parents = {:.2f} ({:.2f}), difficulty = {:.2f} ({:.2f}), Nb/Nc = {:.2f}, block solvetime = {:.2f},".format(mean_parents, SD_parents, mean_d, SD_d, \
             mean_Nb_Nc, mean_st)

    plt.title(title)
    plt.xlabel('height')
    plt.ylabel('difficulty, hashrate, parents, 1/10th Nc_time')
    plt.legend()
    manager = plt.get_current_fig_manager()
    #manager.resize(*manager.window.maxsize())
    plt.show()

def do_all(DAA_,Nb_):
    initialize_global_arrays(DAA_, Nb_)
    print_messages()
    global start, finish_time
    start = timer.time()
    do_mining()
    # Reverse the parents array: compute children
    for b in range(blocks):
        for p in parents[b]:
            children[p] = children[p].union([b])
    finish_time = timer.time() - start
    print_finish_message()
    if show_single_plots: single_plots()
    return x, d, SD_x, SD_d, mean_d, num_parents, SD_parents

def next_generation(bs, older=False):
    """ Returns the set of beads one generation from {bs} in the <older>
        direction using either the parents or children arrays.
    """
    generation = parents if older else children
    if isinstance(bs, int) or isinstance(bs, np.int32): bs = frozenset([bs])
    return frozenset({g for b in bs for g in generation[b]})

def cohorts(initial_cohort=None, older=False):
    """ Given the seed of the next cohort (which is the set of beads one step older, in the next
        cohort), build an ancestor and descendant set for each visited bead.  A new cohort is
        formed if we encounter a set of beads, stepping in the descendant direction, for which
        *all* beads in this cohort are ancestors of the first generation of beads in the next
        cohort.

        This function will not return the tips nor any beads connected to them, that do not yet
        form a cohort, (nor the genesis bead when traversing older=True).

        cohort: frozenset of beads
        head: frozenset of beads on the boundary with the last cohort
        gen: frozenset of beads in the generation under consideration
        parents: dictionary of {bead: frozenset} for the parents of each bead examined
        ancestors: dictionary of {bead: frozenset} for the ancestors of each bead examined
    """
    if isinstance(initial_cohort, int) or isinstance(initial_cohort, np.int32):
        cohort = frozenset([initial_cohort])
    else:
        cohort = initial_cohort or frozenset([0])
    head   = next_generation(cohort, older)
    while True :
        yield cohort
        gen         = head
        cparents    = ancestors = {h: next_generation(h, not older) - cohort for h in head}
        while True:                                                                            # DFS search
            gen = next_generation(gen, older)
            if not gen: return                                                                 # Ends the iteration (StopIteration) at a tip
            for g in gen: cparents[g] = next_generation(g, not older)                          # Collect parents of every bead in this generation
            while True:                                                                        # BFS Update ancestors: parents plus its parents' parents
                oldancestors = {g: ancestors[g] for g in gen}                                  # loop because ancestors may have new ancestors
                for g in gen:
                    if all([p in ancestors for p in cparents[g]]):                             # If we have ancestors for all parents of g,
                        ancestors[g] = cparents[g].union(*[ancestors[p] for p in cparents[g]]) # update the ancestors with other ancestors of g's parents
                if oldancestors == {g: ancestors[g] for g in gen}: break                       # Break if ancestors haven't changed
            if(all([p in ancestors] for p in frozenset.union(*[cparents[g] for g in gen]))     # we have no missing ancestors
                and all([h in ancestors[g] for h in head for g in gen])):                      # and everyone has all head beads as ancestors
                cohort = frozenset.intersection(*[ancestors[g] for g in gen])                  # We found a new cohort
                head = next_generation(cohort, older) - cohort                                 # the youngest beads outside the candidate cohort
                tail = next_generation(head, not older)                                        # the oldest beads in the candidate cohort
                if all([h in ancestors and p in ancestors[h] for h in head for p in tail]):    # yield if all beads in the head are ancestors of all beads
                    break


# This mess is needed for the combined plot
if DAA_0: (x_0, d_0, SD_x_0, SD_d_0, mean_d_0, num_parents_0, SD_parents_0) = do_all(0,Nb_0)
if DAA_1: (x_1, d_1, SD_x_1, SD_d_1, mean_d_1, num_parents_1, SD_parents_1) = do_all(1,Nb_1)
if DAA_2: (x_2, d_2, SD_x_2, SD_d_2, mean_d_2, num_parents_2, SD_parents_2) = do_all(2,Nb_2)

'''
count = [0]*(7)
for i in range(blocks):
    if num_parents[i] == 1: count[0] += 1
    elif num_parents[i] ==2: count[1] += 1
    elif num_parents[i] ==3: count[2] += 1
    elif num_parents[i] ==4: count[3] += 1
    elif num_parents[i] ==5: count[4] += 1
    elif num_parents[i] ==6: count[5] += 1
    elif num_parents[i] ==7: count[6] += 1

for i in range(7): print(count[i]/blocks)

'''
def graph_dag():
    #### Generate DAG graph 
    ll = dag_blocks # number of nodes
    ss = start_dag_height # starting height in parents
    parents2 = [[] for _ in range(ll)]
    for j in range(ss,ss+ll):
        for i in range(len(parents[j])):
            if parents[j][i]-ss < 0: parents2[j-ss].append(0)
            else: parents2[j-ss].append(parents[j][i]-ss)
    nodes = [] ;  edges = [[] for _ in range(ll)]
    for i in range(ll):
        nodes.append(i); edges[i] = parents2[i]; # print(i, edges[i])
    G = nx.DiGraph()
    for node in nodes:  G.add_node(node)
    for i, parent_nodes in enumerate(edges):
        node = i  
        for parent in parent_nodes: G.add_edge(parent, node)
    # Use Graphviz's dot layout for a left-to-right visualization
    try:
        pos = nx.nx_agraph.graphviz_layout(G, prog='dot')
        nx.draw(G, pos, with_labels=True, node_color='lightblue', 
            node_size=200, arrows=True, arrowsize=20)
        plt.show()
    except ImportError:
        print("Please install pygraphviz for this visualization method.")

def combined_plots():
    print("A 2nd plot will pop up after closing the 1st one. The 2nd one may take some time.")
    height = np.arange(0,blocks,1)
    mean_length = 200 # smooth out the variables we're interested in
        
    plt.plot(height, d_0, label='Nb/Nc',    linewidth=2)
    plt.plot(height, d_1, label='Parents',  linewidth=2)
    plt.plot(height, d_2, label='SMA',      linewidth=2)    
    plt.plot(height, latency,   label='latency')
    plt.plot(height, hashrate,  label='hashrate')

    title = "Nb/Nc DAA, Nb = {:.0f}, n = {:.0f}, SD_d = {:.3f}\n".format(Nb_0,n_0, SD_d_0)
    title += "Parent DAA, parents = {:.2f}, n = {:.0f}, SD_d = {:.3f}\n".format(parents_desired, n_1, SD_d_1)
    title += "SMA DAA, block_time = {:.2f}, n = {:.0f}, SD_d = {:.3f}".format(block_time_desired, n_2, SD_d_2)  

    plt.title(title)
    plt.xlabel('height')
    plt.ylabel('difficulty, hashrate, latency')
    plt.legend()
    manager = plt.get_current_fig_manager()
    manager.resize(*manager.window.maxsize())
    plt.show()
    
    DAG_width_0 = [st.mean(num_parents_0)]*(blocks)
    DAG_width_1 = [st.mean(num_parents_1)]*(blocks)
    DAG_width_2 = [st.mean(num_parents_2)]*(blocks)
    for i in range(100,blocks):
        n = min(i,mean_length)
        DAG_width_0[i] = st.mean(num_parents_0[i-n:i])
        DAG_width_1[i] = st.mean(num_parents_1[i-n:i])
        DAG_width_2[i] = st.mean(num_parents_2[i-n:i])
 
    plt.plot(height, DAG_width_0, label='Nb/Nc parents, ' + str(mean_length) + '-block mean', linewidth=2)
    plt.plot(height, DAG_width_1, label='Parent parents, ' + str(mean_length) + '-block mean', linewidth=2)
    plt.plot(height, DAG_width_2, label='SMA parents, ' + str(mean_length) + '-block mean', linewidth=2)    
    plt.plot(height, latency, label='latency')
    plt.plot(height, hashrate, label='hashrate')
  
    title = "Nb/Nc DAA, Nb = {:.0f}, n = {:.0f}, SD_parents = {:.3f}\n".format(Nb_0,n_0, SD_parents_0)
    title += "Parent DAA, parents = {:.2f}, n = {:.0f}, SD_parents = {:.3f}\n".format(parents_desired, n_1, SD_parents_1)
    title += "SMA DAA, block_time = {:.2f}, n = {:.0f}, SD_parents = {:.3f}".format(block_time_desired, n_2, SD_parents_2)

    plt.title(title)
    plt.xlabel('height')
    plt.ylabel('parents, hashrate, latency')
    plt.legend()
    manager = plt.get_current_fig_manager()
    manager.resize(*manager.window.maxsize())
    plt.show()

if show_combined_plots: combined_plots()
if show_dag: graph_dag()

'''
This simulates three PoW DAA's for researching Bob McElrath's Braidpool DAG but has general applicability. The first twp DAA's target a DAG width without using timestamps or estimating hashrate or latency. This is possible because increasing hashrate or latency increases DAG width (average parents per block) to the same extent when the DAG is made as fast as possible. Increasing difficulty has the same effect on both and makes it narrow again. In setting target * hashrate * latency, time cancels. Consensus in Braidpool's scheme is achieved when the DAG width is 1, i.e. the block is a funnel through which all ancestors and descendants are connected. Such a block is the state of the DAG at its point in time. This "1-block cohort" is called an Nc block in this simulator and occurs when the difficulty is hard enough for the current hashrate so that there have not been any blocks mined before or after it within 1 latency period so that all honest parent and child blocks have time to see it. But for this 1-block cohort to occur as fast as possible, the difficulty also needs to be as easy as possible. Bob's Braidpool article discusses the math to try to identify the optimum difficulty to balance these two opposing effects: 
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
Nb_Nc_measured = Nb_setting/(Nc_measured+1.3)
The +1.3 was determined by experiment to more quickly and accurately give 2.42 results when the setting is 2.42, without regard to the Nb setting or filter.

Deprecated DAA calculation for Nb/Nc due to having more variation in x

        if use_approx_DAA:
            if Nb_Nc[h] < 1.5:  CF = 2
            elif Nb_Nc[h] > 3.1:   CF = 0.66**(Nb_Nc[h] /5)
            else:               
                CF = Nb_Nc_desired/(Nb_Nc[h])        
        else: CF = Q/max(0.1,lambertw(complex(max(.1,Nb_Nc[h]-1)),k=0).real)   

        x[h] = x1*(1 + (CF - 1)/n)  

Similarly, methods based on the probability of seeing different numbers of parents in the parent method did not work as well. 
'''

'''
# shows a portion of the DAG as text. lets me check if the parents look right
dd=''
print ("height, solvetime, Nc block?, parents")
for h in range(570,599):
    if Nc_blocks[h]: dd=" == Nc =="
    print("{:.0f}, {:.2} ".format(h,solvetime[h]) + str(parents[h]) + dd)
    dd=''
sys.exit()
'''
