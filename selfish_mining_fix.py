import random
import math

'''
This tests a proposed fix to stop selfish mining. The fix is for nodes to reject a newly-seen block for 2 block times if the 
timestamp of the block is more than 10 seconds into the future or more than 15 seconds in the past compared to the node's local time.
It assumes honest nodes clocks have less than +/- 5 seconds error and propagation delays are a terrible 5 seconds. Results show
selfish miner will have a 16% loss instead of a 15% gain if he has 40% of the network hashrate.
'''

BLOCK_TIME = 150.0
MEDIAN_TIME = 2 * BLOCK_TIME # timeout period if a block violates the rule
PAST_LIMIT = 15.0
FUTURE_LIMIT = 10.0
FUTURE_DELTA_SELFISH = 10.0 # selfish miner future-dates timestamps to try to thwart the rule
PROP_DELAY = 5.0 # netowrk propagation delay
CLOCK_ERROR_MAX = 10.0  # honest-miner clocks randomly vary this much
alpha = 0.4 # selfish mining hashrate proportion

# probability honest miners join the attackers chain if there is a tie and 
# the selfish miner's block timestamps do not violate the rule
gamma = 0.5 

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
                ts_honest = mined_t + random.uniform(0, CLOCK_ERROR_MAX)
                arrival_t = mined_t + PROP_DELAY
                local_t = arrival_t + random.uniform(0, CLOCK_ERROR_MAX)
                bad = False
                if with_rule:
                    if ts_honest > local_t + FUTURE_LIMIT or ts_honest < local_t - PAST_LIMIT:
                        bad = True
                if bad:
                    # Rejection for honest block
                    reject_until = arrival_t + MEDIAN_TIME
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
                arrival_t = mined_t + PROP_DELAY  # Release arrival
                local_t = arrival_t + random.uniform(0, CLOCK_ERROR_MAX)
                if with_rule:
                    for mt in private_mining_times:
                        ts = mt + FUTURE_DELTA_SELFISH
                        if ts > local_t + FUTURE_LIMIT or ts < local_t - PAST_LIMIT:
                            bad = True
                            break
                if bad:
                    reject_until = arrival_t + MEDIAN_TIME
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

sim_time = 10000 * BLOCK_TIME
num_runs = 5
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
