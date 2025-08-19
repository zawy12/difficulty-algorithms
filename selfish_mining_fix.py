import random
import math

'''
This tests a proposed fix to stop selfish mining. The fix is for nodes to reject a newly-seen block for 
X block times if the timestamp of the block is more than Y seconds into the future or more than Z+delay seconds
in the past compared to the node's local time. Also, continue to reject a block if timestamp is greater than 
local time plus a timeout period. It assumes honest nodes clocks have less than +/- X/2 seconds error 
and propagation delays are exponentially-distribution random. Some things in the code will not make
any difference, but they're there in case the simulation is modified to be more complete.
'''

BLOCK_TIME = 150
sim_time = 5000 * BLOCK_TIME 
num_runs = 100
CLOCK_ERROR_MAX = 20.0  # honest miner clocks must have errors less than +/- this value.
TIMEOUT = 2 * BLOCK_TIME # timeout period if a block violates the rule. Try 0.693 to 5.
PROP_DELAY = 0.5 # mean network propagation delay with exponential distribution as experiments show. 0.5 in bitcoin.

alpha = 0.35 # selfish mining hashrate proportion

# probability honest miners join the attackers chain if there is a tie and 
# the selfish miner's block timestamps do not violate the rule
gamma = 0.5

# the following parameters shouldn't need changing as much the the above

FUTURE_LIMIT = 2 * CLOCK_ERROR_MAX # 2x because honest timestamp could be at the +max error and validator at -max error
PAST_LIMIT = 2 * CLOCK_ERROR_MAX + 10*PROP_DELAY  # 10x will be a VERY rare exponential event, allowing more delay hiccups
FUTURE_DELTA_SELFISH = FUTURE_LIMIT # selfish miner future-dates timestamps to try to thwart the rule

def simulate(alpha, gamma, sim_time, with_rule=False):
    t = 0.0
    private_mining_times = []
    private_lead = 0
    state = 0
    longest_chain_length = 0
    num_selfish_blocks = 0
    while t < sim_time:
        dt = random.expovariate(1 / BLOCK_TIME)
        t += dt
        if t > sim_time:
            break
        r = random.random()
        if state == -1:
            if r <= alpha:
                num_selfish_blocks += 2
            elif r <= alpha + (1 - alpha) * gamma:
                num_selfish_blocks += 1
            else:
                num_selfish_blocks += 0
            longest_chain_length += 2
            state = 0
            continue
        if private_lead == 0:
            if r <= alpha:
                private_mining_times = [t]
                private_lead = 1
            else:
                # Honest block mined at t
                mined_t = t
                ts_honest = mined_t + random.uniform(-CLOCK_ERROR_MAX, +CLOCK_ERROR_MAX)
                arrival_t = mined_t + random.expovariate(1/PROP_DELAY)
                local_t = arrival_t + random.uniform(-CLOCK_ERROR_MAX, +CLOCK_ERROR_MAX)
                bad = False
                if with_rule:
                    if ts_honest > local_t + FUTURE_LIMIT or ts_honest < local_t - PAST_LIMIT:
                        bad = True
                if bad:
                    # Rejection for honest block
                    reject_until = max(arrival_t + TIMEOUT,ts_honest-FUTURE_LIMIT) # it never occurs in this code but don't accept if its too fare in future
                    honest_height = 1  # The rejected one starts a side
                    other_height = 0   # No other chain
                    current_t = arrival_t
                    while current_t < reject_until:
                        dt_h = random.expovariate((1 - alpha) / BLOCK_TIME)
                        dt_s = random.expovariate(alpha / BLOCK_TIME)
                        dt = min(dt_h, dt_s)
                        if current_t + dt >= reject_until:
                            break
                        current_t += dt
                        if dt == dt_h:
                            honest_height +=1
                        else:
                            # Selfish mines on main, assume
                            other_height +=1
                    if honest_height > other_height:
                        longest_chain_length += honest_height
                    elif honest_height < other_height:
                        longest_chain_length += other_height
                        num_selfish_blocks += other_height
                    else:
                        # tie, simulate race
                        dt = random.expovariate(1 / BLOCK_TIME)
                        t += dt
                        r = random.random()
                        if r <= alpha:
                            num_selfish_blocks += other_height + 1
                        elif r <= alpha + (1 - alpha) * gamma:
                            num_selfish_blocks += other_height
                        else:
                            num_selfish_blocks += 0
                        longest_chain_length += other_height + 1
                else:
                    longest_chain_length += 1
        else:
            if r <= alpha:
                private_mining_times.append(t)
                private_lead +=1
            else:
                # Honest mined at t, selfish releases
                mined_t = t
                # Check for selfish chain
                bad = False
                arrival_t = mined_t +  random.expovariate(1/PROP_DELAY)  # Release arrival
                local_t = arrival_t + random.uniform(-CLOCK_ERROR_MAX, +CLOCK_ERROR_MAX)
                if with_rule:
                    for mt in private_mining_times:
                        ts = mt + FUTURE_DELTA_SELFISH
                        if ts > local_t + FUTURE_LIMIT or ts < local_t - PAST_LIMIT:
                            bad = True
                            break
                if bad:
                    reject_until = max(arrival_t + TIMEOUT,ts-FUTURE_LIMIT)
                    honest_height = 1  # Honest starts from the find
                    private_height = private_lead
                    current_t = arrival_t
                    while current_t < reject_until:
                        dt_h = random.expovariate((1 - alpha) / BLOCK_TIME)
                        dt_s = random.expovariate(alpha / BLOCK_TIME)
                        dt = min(dt_h, dt_s)
                        if current_t + dt >= reject_until:
                            break
                        current_t += dt
                        if dt == dt_h:
                            honest_height +=1
                        else:
                            private_height +=1
                    if private_height > honest_height:
                        num_selfish_blocks += private_height
                        longest_chain_length += private_height
                    elif private_height < honest_height:
                        longest_chain_length += honest_height
                    else:
                        # tie, simulate race
                        dt = random.expovariate(1 / BLOCK_TIME)
                        t += dt
                        r = random.random()
                        if r <= alpha:
                            num_selfish_blocks += private_height + 1
                        elif r <= alpha + (1 - alpha) * gamma:
                            num_selfish_blocks += private_height
                        else:
                            num_selfish_blocks += 0
                        longest_chain_length += private_height + 1
                else:
                    if private_lead == 1:
                        state = -1
                    else:
                        num_selfish_blocks += private_lead
                        longest_chain_length += private_lead
                private_lead = 0
                private_mining_times = []
    return num_selfish_blocks / longest_chain_length if longest_chain_length > 0 else 0.0

without_sum = 0
with_sum = 0
for _ in range(num_runs):
  without_rule = simulate(alpha, gamma, sim_time, with_rule=False)
  without_sum += without_rule
  with_rule = simulate(alpha, gamma, sim_time, with_rule=True)
  with_sum += with_rule
avg_without = without_sum / num_runs
avg_with = with_sum / num_runs
gain_without = (avg_without - alpha) / alpha * 100
gain_with = (avg_with - alpha) / alpha * 100
print("Without rule: {:.2f}% {}".format(abs(gain_without), "gain" if gain_without > 0 else "loss"))
print("With rule: {:.2f}% {}".format(abs(gain_with), "gain" if gain_with > 0 else "loss"))
