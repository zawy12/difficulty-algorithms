
/*   
Preliminary code for super-fast increases in difficulty.   
Requires the ability to change the difficulty during the current block,
based on the timestamp the miner selects. See my github issue #36 and KMD.
Needs intr-block exponential decay function because 
this can make difficulty jump very high.
Miners need to caclulate new difficulty with each second, or
maybe 3 seconds.  FTL, MTP, and revert to local times must be small. 
MTP=1 if using Digishield. Out-of-sequence timestamps must be forbidden.

1) bnTarget = Digishield() or other baseline DA 
2) bnTarget = RT_CST_RST() 
3) bnTarget = max(bnTarget,expdecay())

RT_CST_RST() multiplies Recent Target(s), Current Solvetimes, & 
Recent SolveTime if RST had an unlikely 1/200 block chance of 
being too fast on accident. This estimates and adjusts for recent 
hashrate aggressively (lots of random error) but corrects the error by
CST adjusting the difficulty during the block.

It checks to see if there was an "active trigger" still in play which
occurs when recent block emission rate has been too fast. Triggers 
are supposed to be active if emission rate has not slowed up enough
to get back on track. It checks the longest range first because it's
the least aggressive.

T = target blocktime 
ts = timestamp vector, 62 elements, 62 is oldest  (elements needed are 50+W)
ct = cumulative targets, 62 elements, 62 is oldest
W = window size of recent solvetimes and targets to use that estimates hashrate
numerator & deonominator needed for 1/200 possion estimator
past = how far back in past to look for beginning of a trigger

*/

// create ts and cw vectors
// Get bnTarget = Digishield();

arith_uint256 past = 50;

arith_uint256 W = 12;
arith_uint256 numerator = 12;
arith_uint256 denominator = 7;

// bnTarget = RT_CST_RST (bnTarget, ts, cw, numerator, denominator, W, T, past);

W = 6; top = 7; denominator = 3;

// bnTarget = RT_CST_RST (bnTarget, ts, cw, numerator, denominator, W, T, past);

W = 3; top = 1; denominator = 2;

bnTarget = RT_CST_RST (bnTarget, ts, cw, numerator, denominator, W, T, past);

arith_uint256 RT_CST_RST ( arith_uint256 bnTarget, std::vector<arith_uint256> ts, std::vector<arith_uint256> ct, 
							arith_uint256 numerator, arith_uint256 denominator, arith_uint256 W, arith_uint256 T, arith_uint256 past  ) {

	if (ts.size() < 2*W || ct.size() < 2*W ) { exit; } // error. a vector was too small 
	if (ts.size() < past+W || ct.size() < past+W ) { past = min(ct.size(), ts.size()) - W; } // past was too small, adjust

	arith_uint256 K = 1e6, i, j, ii=0; // K is a scaling factor for integer divisions

	if ( ts[-1]-ts[-W] < T*numerator/denominator ) { 
		bnTarget = ((ct[0]-ct[1])/K)*max(K,(K*(nTime-ts[0])*(ts[0]-ts[W])*denominator/numerator)/T/T);  
	}

	/*  Check past 24 blocks for any sum of 3 STs < T/2 triggers. This is messy
	because the blockchain does not allow us to store a variable to know
	if we are currently in a triggered state that is making a sequence of 
	adjustments to prevTargets, so we have to look for them.

	Nested loops do this: if block emission has not slowed to be back on track at 
	any time since most recent trigger and we are at current block, aggressively
	adust prevTarget. */

	for ( int j=past-1; j<= 1; j--) {
		if ( ts[j]-ts[j-W] < T*numerator/denominator ) {
			for ( int i = j; i <= 0; i-- ) {
				ii++;
				// Check if emission caught up. If yes, "trigger stopped at i". 
				// Break loop to try more recent j's to see if trigger activates again.
				if ( ts[i+1]-ts[j-W] > (ii+W)*T ) { break; } 
			
				/* We're here, so there was a TS[j]-TS[j-3] < T/2 trigger in the past 
				and emission rate has not yet slowed up to be back on track so the 
				"trigger is still active", aggressively adjusting target here at block "i" . */

				if (i == 0) { 
				
					/* We made it all the way to current block. Emission rate since
					last trigger never slowed enough to get back on track, so adjust again.
					If avg last 3 STs = T, this increases target to prevTarget as ST increases to T. 
					This biases it towards ST=~1.75*T to get emission back on track.
					If avg last 3 STs = T/2, target increases to prevTarget at 2*T.
					Rarely, last 3 STs can be 1/2 speed => target = prevTarget at T/2, & 1/2 at T.*/

					bnTarget = ((ct[0]-ct[W])/W/K)*(K*(nTime-TS[0])*(ts[0]-ts[W]))/W/T/T); 
					j=0; // It needed adjusting, we adjusted it, we're finished, so break out of j loop.
				}
			}
		}
	}
return bnTarget;
}

