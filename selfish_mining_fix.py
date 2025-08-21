import random
import math
from collections import Counter

'''
This is the same as the classic selfish mining script that orphans only 1 honest block at a time, but
it allows 3 variants of the "stubborn" strategy that causes deep reorgs.

This tests a proposed fix to stop selfish mining. The fix is for nodes to reject a newly-seen block for 
X block times if the timestamp of the block is more than Y seconds into the future or more than Z+delay seconds
in the past compared to the node's local time. Also, continue to reject a block if timestamp is greater than 
local time plus a timeout period. It assumes honest nodes clocks have less than +/- X/2 seconds error 
and propagation delays are exponentially-distribution random. Some things in the code will not make
any difference, but they're there in case the simulation is modified to be more complete.
'''

BLOCK_TIME = 150
sim_time = 5760 * BLOCK_TIME # 576 blocks / day 
num_runs = 100
CLOCK_ERROR_MAX = 20.0  # honest miner clocks must have errors less than +/- this value.
TIMEOUT = 2 * BLOCK_TIME # timeout period if a block violates the rule. Try 0.693 to 5.
PROP_DELAY = 0.5 # mean network propagation delay with exponential distribution as experiments show. 0.5 in bitcoin.

strategy = 'classic'  # 'classic', 'trail-stubborn', 'lead-stubborn', 'equal-fork-stubborn'

alpha = 0.40 # selfish mining hashrate proportion

# probability honest miners join the attackers chain if there is a tie and 
# the selfish miner's block timestamps do not violate the rule
gamma = 0.5

enforce_fraction = 1.0  # fraction of honest miners that is enforcing
enforce_fraction = alpha
enforce_hashrate = 0.5 
enforce_fraction = enforce_hashrate / (1-alpha)

# the following parameters shouldn't need changing as much the the above

FUTURE_LIMIT = 2 * CLOCK_ERROR_MAX # 2x because honest timestamp could be at the +max error and validator at -max error
PAST_LIMIT = 2 * CLOCK_ERROR_MAX + 10*PROP_DELAY  # 10x will be a VERY rare exponential event, allowing more delay hiccups
FUTURE_DELTA_SELFISH = FUTURE_LIMIT # selfish miner future-dates timestamps to try to thwart the rule

def simulate(alpha, gamma, sim_time, with_rule=False, enforce_fraction=1.0):
    if strategy == 'classic':
        trail_stubborn = 0
        lead_stubborn = False
        equal_fork_stubborn = False
    elif strategy == 'trail-stubborn':
        trail_stubborn = math.inf
        lead_stubborn = False
        equal_fork_stubborn = False
    elif strategy == 'lead-stubborn':
        trail_stubborn = 0
        lead_stubborn = True
        equal_fork_stubborn = False
    elif strategy == 'equal-fork-stubborn':
        trail_stubborn = 0
        lead_stubborn = False
        equal_fork_stubborn = True
    else:
        raise ValueError("Invalid strategy")

    t = 0.0
    private_mining_times = []
    private_lead = 0
    state = 0
    longest_chain_length = 0
    num_selfish_blocks = 0

    attacker_reorg_sizes = Counter()
    honest_reorg_sizes = Counter()

    honest_branch_size = 0
    released_time = 0.0
    is_trailing = False

    while t < sim_time:
        if is_trailing:
            dt = random.expovariate(1 / BLOCK_TIME)
            t += dt
            if t > sim_time:
                break
            r = random.random()
            if r <= alpha:
                private_mining_times.append(t)
                private_lead +=1
                if private_lead > 0:
                    bad = False
                    arrival_t = t + random.expovariate(1 / PROP_DELAY)
                    local_t = arrival_t + random.uniform(-CLOCK_ERROR_MAX, +CLOCK_ERROR_MAX)
                    if with_rule:
                        for mt in private_mining_times:
                            ts = mt + FUTURE_DELTA_SELFISH
                            if ts > local_t + FUTURE_LIMIT or ts < local_t - PAST_LIMIT:
                                bad = True
                                break
                    if bad:
                        reject_until = max(arrival_t + TIMEOUT, private_mining_times[-1] + FUTURE_DELTA_SELFISH - FUTURE_LIMIT)
                        honest_height = honest_branch_size
                        private_height = len(private_mining_times)
                        num_selfish_private = len(private_mining_times)
                        current_t = arrival_t
                        h = 1 - alpha
                        beta = enforce_fraction
                        enf_hash = beta * h
                        non_enf_hash = (1 - beta) * h
                        while current_t < reject_until:
                            dt_enforce = random.expovariate(enf_hash / BLOCK_TIME) if enf_hash > 0 else math.inf
                            dt_non = random.expovariate(non_enf_hash / BLOCK_TIME) if non_enf_hash > 0 else math.inf
                            dt_self = random.expovariate(alpha / BLOCK_TIME) if alpha > 0 else math.inf
                            dt = min(dt_enforce, dt_non, dt_self)
                            if current_t + dt >= reject_until:
                                break
                            current_t += dt
                            if dt == dt_enforce:
                                honest_height +=1
                            elif dt == dt_non:
                                private_height +=1
                            else:
                                private_height +=1
                                num_selfish_private +=1
                        if private_height > honest_height:
                            num_selfish_blocks += num_selfish_private
                            longest_chain_length += private_height
                            attacker_reorg_sizes[honest_height] += 1
                        elif private_height < honest_height:
                            longest_chain_length += honest_height
                            honest_reorg_sizes[private_height] += 1
                        else:
                            effective_gamma = gamma * (1 - enforce_fraction)
                            dt = random.expovariate(1 / BLOCK_TIME)
                            t += dt
                            r = random.random()
                            if r <= alpha:
                                num_selfish_blocks += num_selfish_private + 1
                            elif r <= alpha + (1 - alpha) * effective_gamma:
                                num_selfish_blocks += num_selfish_private
                            else:
                                num_selfish_blocks += 0
                            longest_chain_length += private_height + 1
                            if r <= alpha + (1 - alpha) * effective_gamma:
                                attacker_reorg_sizes[honest_height] += 1
                            else:
                                honest_reorg_sizes[private_height] += 1
                    else:
                        num_selfish_blocks += len(private_mining_times)
                        longest_chain_length += len(private_mining_times)
                        attacker_reorg_sizes[honest_branch_size] += 1
                    private_lead = 0
                    private_mining_times = []
                    is_trailing = False
            else:
                private_lead -=1
                honest_branch_size +=1
                if -private_lead > trail_stubborn:
                    honest_reorg_sizes[len(private_mining_times)] += 1
                    longest_chain_length += honest_branch_size
                    private_lead = 0
                    private_mining_times = []
                    is_trailing = False
            continue
        dt = random.expovariate(1 / BLOCK_TIME)
        t += dt
        if t > sim_time:
            break
        r = random.random()
        if state == -1:
            if r <= alpha:
                if equal_fork_stubborn:
                    private_mining_times.append(t)
                    private_lead +=1
                    continue
                else:
                    num_selfish_blocks += 2
                    longest_chain_length += 2
                    attacker_reorg_sizes[1] += 1
            elif r <= alpha + (1 - alpha) * gamma:
                num_selfish_blocks += 1
                longest_chain_length += 2
                attacker_reorg_sizes[1] += 1
            else:
                if trail_stubborn > 0:
                    private_mining_times = [released_time]
                    private_lead = -1
                    honest_branch_size = 2
                    is_trailing = True
                    continue
                else:
                    longest_chain_length += 2
                    honest_reorg_sizes[1] += 1
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
                        honest_reorg_sizes[other_height] += 1
                    elif honest_height < other_height:
                        longest_chain_length += other_height
                        num_selfish_blocks += other_height
                        attacker_reorg_sizes[honest_height] += 1
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
                        if r <= alpha + (1 - alpha) * gamma:
                            attacker_reorg_sizes[honest_height] += 1
                        else:
                            honest_reorg_sizes[other_height] += 1
                else:
                    if trail_stubborn > 0:
                        private_lead = -1
                        honest_branch_size = 1
                        private_mining_times = []
                        is_trailing = True
                    else:
                        longest_chain_length += 1
            continue
        else:
            if r <= alpha:
                private_mining_times.append(t)
                private_lead +=1
            else:
                # Honest mined at t, selfish releases
                mined_t = t
                arrival_t = mined_t + random.expovariate(1/PROP_DELAY)  # Release arrival
                local_t = arrival_t + random.uniform(-CLOCK_ERROR_MAX, +CLOCK_ERROR_MAX)
                bad = False
                if with_rule:
                    for mt in private_mining_times:
                        ts = mt + FUTURE_DELTA_SELFISH
                        if ts > local_t + FUTURE_LIMIT or ts < local_t - PAST_LIMIT:
                            bad = True
                            break
                if bad:
                    reject_until = max(arrival_t + TIMEOUT, private_mining_times[-1] + FUTURE_DELTA_SELFISH - FUTURE_LIMIT)
                    honest_height = 1  # Honest starts from the find
                    private_height = private_lead
                    num_selfish_private = private_lead
                    current_t = arrival_t
                    h = 1 - alpha
                    beta = enforce_fraction
                    enf_hash = beta * h
                    non_enf_hash = (1 - beta) * h
                    while current_t < reject_until:
                        dt_enforce = random.expovariate(enf_hash / BLOCK_TIME) if enf_hash > 0 else math.inf
                        dt_non = random.expovariate(non_enf_hash / BLOCK_TIME) if non_enf_hash > 0 else math.inf
                        dt_self = random.expovariate(alpha / BLOCK_TIME) if alpha > 0 else math.inf
                        dt = min(dt_enforce, dt_non, dt_self)
                        if current_t + dt >= reject_until:
                            break
                        current_t += dt
                        if dt == dt_enforce:
                            honest_height +=1
                        elif dt == dt_non:
                            private_height +=1
                        else:
                            private_height +=1
                            num_selfish_private +=1
                    if private_height > honest_height:
                        num_selfish_blocks += num_selfish_private
                        longest_chain_length += private_height
                        attacker_reorg_sizes[honest_height] += 1
                    elif private_height < honest_height:
                        longest_chain_length += honest_height
                        honest_reorg_sizes[private_height] += 1
                    else:
                        effective_gamma = gamma * (1 - enforce_fraction)
                        dt = random.expovariate(1 / BLOCK_TIME)
                        t += dt
                        r = random.random()
                        if r <= alpha:
                            num_selfish_blocks += num_selfish_private + 1
                        elif r <= alpha + (1 - alpha) * effective_gamma:
                            num_selfish_blocks += num_selfish_private
                        else:
                            num_selfish_blocks += 0
                        longest_chain_length += private_height + 1
                        if r <= alpha + (1 - alpha) * effective_gamma:
                            attacker_reorg_sizes[honest_height] += 1
                        else:
                            honest_reorg_sizes[private_height] += 1
                    private_lead = 0
                    private_mining_times = []
                else:
                    if private_lead == 1:
                        bad = False
                        if with_rule:
                            ts = private_mining_times[0] + FUTURE_DELTA_SELFISH
                            if ts > local_t + FUTURE_LIMIT or ts < local_t - PAST_LIMIT:
                                bad = True
                        if bad:
                            reject_until = max(arrival_t + TIMEOUT, ts - FUTURE_LIMIT)
                            honest_height = 1
                            private_height = 1
                            num_selfish_private = 1
                            current_t = arrival_t
                            h = 1 - alpha
                            beta = enforce_fraction
                            enf_hash = beta * h
                            non_enf_hash = (1 - beta) * h
                            while current_t < reject_until:
                                dt_enforce = random.expovariate(enf_hash / BLOCK_TIME) if enf_hash > 0 else math.inf
                                dt_non = random.expovariate(non_enf_hash / BLOCK_TIME) if non_enf_hash > 0 else math.inf
                                dt_self = random.expovariate(alpha / BLOCK_TIME) if alpha > 0 else math.inf
                                dt = min(dt_enforce, dt_non, dt_self)
                                if current_t + dt >= reject_until:
                                    break
                                current_t += dt
                                if dt == dt_enforce:
                                    honest_height +=1
                                elif dt == dt_non:
                                    private_height +=1
                                else:
                                    private_height +=1
                                    num_selfish_private +=1
                            if private_height > honest_height:
                                num_selfish_blocks += num_selfish_private
                                longest_chain_length += private_height
                                attacker_reorg_sizes[honest_height] += 1
                                private_lead = 0
                                private_mining_times = []
                            elif private_height < honest_height:
                                longest_chain_length += honest_height
                                honest_reorg_sizes[private_height] += 1
                                private_lead = 0
                                private_mining_times = []
                            else:
                                effective_gamma = gamma * (1 - enforce_fraction)
                                dt = random.expovariate(1 / BLOCK_TIME)
                                t += dt
                                r = random.random()
                                if r <= alpha:
                                    num_selfish_blocks += num_selfish_private + 1
                                elif r <= alpha + (1 - alpha) * effective_gamma:
                                    num_selfish_blocks += num_selfish_private
                                else:
                                    num_selfish_blocks += 0
                                longest_chain_length += private_height + 1
                                if r <= alpha + (1 - alpha) * effective_gamma:
                                    attacker_reorg_sizes[honest_height] += 1
                                else:
                                    honest_reorg_sizes[private_height] += 1
                                private_lead = 0
                                private_mining_times = []
                        else:
                            released_time = private_mining_times[0]
                            state = -1
                            private_lead = 0
                            private_mining_times = []
                    else:
                        if lead_stubborn:
                            bad = False
                            if with_rule:
                                ts = private_mining_times[0] + FUTURE_DELTA_SELFISH
                                if ts > local_t + FUTURE_LIMIT or ts < local_t - PAST_LIMIT:
                                    bad = True
                            if bad:
                                reject_until = max(arrival_t + TIMEOUT, ts - FUTURE_LIMIT)
                                honest_height = 1
                                private_height = 1
                                num_selfish_private = 1
                                current_t = arrival_t
                                h = 1 - alpha
                                beta = enforce_fraction
                                enf_hash = beta * h
                                non_enf_hash = (1 - beta) * h
                                while current_t < reject_until:
                                    dt_enforce = random.expovariate(enf_hash / BLOCK_TIME) if enf_hash > 0 else math.inf
                                    dt_non = random.expovariate(non_enf_hash / BLOCK_TIME) if non_enf_hash > 0 else math.inf
                                    dt_self = random.expovariate(alpha / BLOCK_TIME) if alpha > 0 else math.inf
                                    dt = min(dt_enforce, dt_non, dt_self)
                                    if current_t + dt >= reject_until:
                                        break
                                    current_t += dt
                                    if dt == dt_enforce:
                                        honest_height +=1
                                    elif dt == dt_non:
                                        private_height +=1
                                    else:
                                        private_height +=1
                                        num_selfish_private +=1
                                if private_height > honest_height:
                                    num_selfish_blocks += num_selfish_private
                                    longest_chain_length += private_height
                                    attacker_reorg_sizes[honest_height] += 1
                                    private_lead -= 1
                                    private_mining_times = private_mining_times[1:]
                                elif private_height < honest_height:
                                    longest_chain_length += honest_height
                                    honest_reorg_sizes[private_height] += 1
                                    private_lead = 0
                                    private_mining_times = []
                                else:
                                    effective_gamma = gamma * (1 - enforce_fraction)
                                    dt = random.expovariate(1 / BLOCK_TIME)
                                    t += dt
                                    r = random.random()
                                    if r <= alpha:
                                        num_selfish_blocks += num_selfish_private + 1
                                    elif r <= alpha + (1 - alpha) * effective_gamma:
                                        num_selfish_blocks += num_selfish_private
                                    else:
                                        num_selfish_blocks += 0
                                    longest_chain_length += private_height + 1
                                    if r <= alpha + (1 - alpha) * effective_gamma:
                                        attacker_reorg_sizes[honest_height] += 1
                                    else:
                                        honest_reorg_sizes[private_height] += 1
                                    private_lead -= 1
                                    private_mining_times = private_mining_times[1:]
                            else:
                                released_time = private_mining_times[0]
                                private_mining_times = private_mining_times[1:]
                                private_lead -=1
                                state = -1
                        else:
                            num_selfish_blocks += private_lead
                            longest_chain_length += private_lead
                            attacker_reorg_sizes[1] += 1
                            private_lead = 0
                            private_mining_times = []
    ratio = num_selfish_blocks / longest_chain_length if longest_chain_length > 0 else 0.0
    return ratio, attacker_reorg_sizes, honest_reorg_sizes

without_sum = 0
without_attacker_sizes = Counter()
without_honest_sizes = Counter()
with_sum = 0
with_attacker_sizes = Counter()
with_honest_sizes = Counter()
for _ in range(num_runs):
  without_rule, a_sizes, h_sizes = simulate(alpha, gamma, sim_time, with_rule=False, enforce_fraction=enforce_fraction)
  without_sum += without_rule
  without_attacker_sizes += a_sizes
  without_honest_sizes += h_sizes
  with_rule, a_sizes, h_sizes = simulate(alpha, gamma, sim_time, with_rule=True, enforce_fraction=enforce_fraction)
  with_sum += with_rule
  with_attacker_sizes += a_sizes
  with_honest_sizes += h_sizes
avg_without = without_sum / num_runs
avg_with = with_sum / num_runs
gain_without = (avg_without - alpha) / alpha * 100
gain_with = (avg_with - alpha) / alpha * 100
print("Without rule: {:.2f}% {}".format(abs(gain_without), "gain" if gain_without > 0 else "loss"))
print("With rule: {:.2f}% {}".format(abs(gain_with), "gain" if gain_with > 0 else "loss"))
print("Total attacker blocks orphaned  without rule:")
for size in sorted(without_honest_sizes):
    print("Size {}: {}".format(size, without_honest_sizes[size]))
print("\nTotal honest blocks orphaned without rule:")
for size in sorted(without_attacker_sizes):
    print("Size {}: {}".format(size, without_attacker_sizes[size]))
print("\nTotal attacker blocks orphaned with rule:")
for size in sorted(with_honest_sizes):
    print("Size {}: {}".format(size, with_honest_sizes[size]))
print("\nTotal honest blocks orphaned with rule:")
for size in sorted(with_attacker_sizes):
    print("Size {}: {}".format(size, with_attacker_sizes[size]))
