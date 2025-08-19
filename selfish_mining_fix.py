import random

'''
This tests a proposed fix to stop selfish mining. The fix is for nodes to reject a newly-seen block for 
2 block times if the timestamp of the block is more than 10 seconds into the future or more than 15 seconds
in the past compared to the node's local time. Also, continue to reject a block if timestamp is greater than 
local time plus the 2-block timeout period. It assumes honest nodes clocks have less than +/- 5 seconds error 
and median propagation delays are 0.5 seconds but subject to 10 s delays. Results show a selfish miner will have a 1% gain instead of a 
15% gain if he has 40% of the network hashrate.
'''

BLOCK_TIME = 150
sim_time = 1152 * BLOCK_TIME  # 1152 = 2 days at 2.5 minutes
num_runs = 100
CLOCK_ERROR_MAX = 10.0  # honest miner clocks must have errors less than +/- half this value.
PROP_DELAY = 0.5  # median network propagation delay. 0.5 in bitcoin. (Note: comment suggests 5 seconds, adjust if needed)
PAST_LIMIT = 2 * CLOCK_ERROR_MAX + 20 * PROP_DELAY  # 20 * to make allowance for miners who aren't well-connected
FUTURE_LIMIT = 2 * CLOCK_ERROR_MAX
TIMEOUT = 2 * BLOCK_TIME  # timeout period if a block violates the rule. try 0.693 to 5.
FUTURE_DELTA_SELFISH = FUTURE_LIMIT  # selfish miner future-dates timestamps to try to thwart the rule

alpha = 0.40  # selfish mining hashrate proportion

# probability honest miners join the attackers chain if there is a tie and 
# the selfish miner's block timestamps do not violate the rule
gamma = 0.4 

def simulate_race(t_start, t_end, honest_rate, selfish_rate, h_height, s_height):
    current_t = t_start
    ha = h_height
    hb = s_height
    while current_t < t_end:
        dt_h = random.expovariate(honest_rate / BLOCK_TIME)
        dt_s = random.expovariate(selfish_rate / BLOCK_TIME)
        dt = min(dt_h, dt_s)
        if current_t + dt >= t_end:
            break
        current_t += dt
        if dt == dt_h:
            ha += 1
        else:
            hb += 1
    return current_t, ha, hb

def handle_rejection(arrival_t, bad_future, max_ts, clock_error, is_rejected_selfish, rejected_init, main_init, alpha, block_time, timeout, gamma):
    reject_until = arrival_t + timeout
    if is_rejected_selfish:
        h_init = main_init  # honest mines on main
        s_init = rejected_init  # selfish mines on rejected
    else:
        h_init = rejected_init  # honest mines on rejected
        s_init = main_init  # selfish mines on main
    current_t, ha, hb = simulate_race(arrival_t, reject_until, 1 - alpha, alpha, h_init, s_init)
    
    rejected_leading = (hb > ha) if is_rejected_selfish else (ha > hb)
    if bad_future and rejected_leading:
        remaining = max_ts - FUTURE_LIMIT - clock_error - current_t
        while remaining > 0:
            dt_h = random.expovariate((1 - alpha) / block_time)
            dt_s = random.expovariate(alpha / block_time)
            dt = min(dt_h, dt_s, remaining)
            current_t += dt
            remaining -= dt
            if dt < remaining:
                if dt == dt_h:
                    ha += 1
                else:
                    hb += 1
    
    if ha > hb:
        added_longest = ha
        added_selfish = 0
    elif hb > ha:
        added_longest = hb
        added_selfish = hb
    else:
        # tie, simulate race
        dt = random.expovariate(1 / block_time)
        current_t += dt
        r = random.random()
        added_longest = ha + 1
        if r <= alpha:
            added_selfish = hb + 1
        elif r <= alpha + (1 - alpha) * gamma:
            added_selfish = hb
        else:
            added_selfish = 0
    
    return current_t, added_longest, added_selfish

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
        is_selfish_mine = (r <= alpha)
        mined_t = t
        
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
            if is_selfish_mine:
                private_mining_times = [mined_t]
                private_lead = 1
            else:
                # Honest block mined
                ts_honest = mined_t + random.uniform(0, CLOCK_ERROR_MAX)
                arrival_t = mined_t + PROP_DELAY
                clock_error = random.uniform(0, CLOCK_ERROR_MAX)
                local_t = arrival_t + clock_error
                bad = False
                bad_future = False
                if with_rule:
                    if ts_honest > local_t + FUTURE_LIMIT:
                        bad = True
                        bad_future = True
                    if ts_honest < local_t - PAST_LIMIT:
                        bad = True
                if not bad:
                    longest_chain_length += 1
                else:
                    # Handle rejection for honest block
                    t, add_l, add_s = handle_rejection(
                        arrival_t, bad_future, ts_honest, clock_error, 
                        is_rejected_selfish=False, rejected_init=1, main_init=0, 
                        alpha=alpha, block_time=BLOCK_TIME, timeout=TIMEOUT, gamma=gamma
                    )
                    longest_chain_length += add_l
                    num_selfish_blocks += add_s
        else:
            if is_selfish_mine:
                private_mining_times.append(mined_t)
                private_lead += 1
            else:
                # Honest mined, selfish releases chain
                arrival_t = mined_t + PROP_DELAY
                clock_error = random.uniform(0, CLOCK_ERROR_MAX)
                local_t = arrival_t + clock_error
                bad = False
                bad_future = False
                max_ts = max(private_mining_times) + FUTURE_DELTA_SELFISH
                if with_rule:
                    if max_ts > local_t + FUTURE_LIMIT:
                        bad = True
                        bad_future = True
                    for mt in private_mining_times:
                        ts = mt + FUTURE_DELTA_SELFISH
                        if ts < local_t - PAST_LIMIT:
                            bad = True
                            break
                if not bad:
                    if private_lead == 1:
                        state = -1
                    else:
                        num_selfish_blocks += private_lead
                        longest_chain_length += private_lead
                else:
                    # Handle rejection for selfish chain
                    t, add_l, add_s = handle_rejection(
                        arrival_t, bad_future, max_ts, clock_error, 
                        is_rejected_selfish=True, rejected_init=private_lead, main_init=1, 
                        alpha=alpha, block_time=BLOCK_TIME, timeout=TIMEOUT, gamma=gamma
                    )
                    longest_chain_length += add_l
                    num_selfish_blocks += add_s
                private_lead = 0
                private_mining_times = []
    return num_selfish_blocks / longest_chain_length if longest_chain_length > 0 else 0.0

without_sum = 0.0
with_sum = 0.0
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
