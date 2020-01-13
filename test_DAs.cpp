/*
Copyright (c) 2018-2020 by Zawy, MIT license.
This tests various difficulty algorithms. Unfortunately, the algorithms are terms of difficulty instead of targets. 
This is a consequence of initially being for CryptoNote-type code. It's complicated because it's 
flexible. See main() to select algorithm(s) and settings. It can simulate on-off mining. It outputs 
timestamps and difficulties to screen, file, and gnuplot. There are no inputs to the program except what's in main().
You compile and run this then refresh "test_DAs.html" in a browser to see the output. I compile and run this with: 
g++ -std=c++11 test_DAs.cpp -o test_DAs && ./test_DAs

Several of the algorithms get average target by using the harmonic mean of difficulties which uses a 1E13 factor
that may cause overflow or underflow depending on difficulty, T, and N. Difficulty is usuallly ok in the range 1E4 to 1E13.
Therefore, several of the algorithms are not "production ready". A coin that adjusts targets based on past targets 
instead of past difficulties should not use any of this code directly, but rewrite it with inverse math.
*/

#include <iostream> // needed for cout << endl.
#include <vector>
#include <fstream> 
#include <bits/stdc++.h> // for sorting vectors
#include <string> 
#include <math.h>  // needed for log()
#include <cassert>  // wownero said this was needed

// This is supposed to be a bad idea that reduces clutter.
using namespace std;

typedef uint64_t u;

string temp = "test_DAs.html";
ofstream html_file(temp);

// Allow some global constants for all simulations.
// These are set in main() so that there's only 1 place to change variables
vector<float>NEG_LOG_RAND(0);
u BLOCKS, FORK_HEIGHT, START_TIMESTAMP, START_CD, BASELINE_D, USE_CN_DELAY, DX, ENABLE_FILE_WRITES, PRINT_BLOCKS_TO_COMMAND_LINE; 
// The following are for TSA
u CONSTANT_HR(1), HR_NEW_METHOD(1), IDENTIFIER(0), R=4;

float fRand(float fMin, float fMax) {	
		float f = (float)rand() / RAND_MAX;
	 return fMin + f * (fMax - fMin);
}

simulate_ST (u D, u DX, u HR_base, u T, u HR_profile) { }

u exponential_function_for_integers (u x_times_1M) {
	// This calculates e^x without decimals by passing it an integer x_times_1M and getting 
	// in return 10,000*e^(x_times_1M/1E6). e^24,9999 (x=24.9999) is highest value it accepts. 
	// Error is < +/-0.015%, avg = +/- 0.005%.  The math: let x = N.nnnn so that e^x = e^N * e^0.nnnn because series expansion 
	// for e^0.nnnn is a lot more accurate than e^N values which can be more easily stored or calculated. 
	u k = 1e6, R=0;
	u exx = k;
	// These are 1000x larger which results in output being 1000x larger
	u exp_of_integers[25] = { 10000, 27183, 73891, 200855, 545982, 1484132, 4034288, 10966332, 29809580, 81030839, 
				220264658, 598741417, 1627547914, 4424133920, 12026042842, 32690173725, 88861105205, 241549527536, 
				656599691373, 1784823009632, 4851651954098, 13188157344832, 35849128461316, 97448034462489, 264891221298435 };
	// for (u i = 1; i <= x_times_10k/k; i++ ) { exx = (exx*27183)/k; } // loop method can replace the above if k is changed

	if ( x_times_1M/k > 17 ) { exx = exp_of_integers[17]; } 
	else { exx = exp_of_integers[x_times_1M/k]; } 
	
	R = x_times_1M % k; // remainder 
	exx = (exx*(k+(R*(k+(R*(k+(R*(k+(R*(k+(R*(k+(R*k)/6/k))/5/k))/4/k))/3/k))/2/k))/k))/(k-1); // (k-1) fudge helped center error.
	return exx;
}

u harmonic_mean (vector<u>cumulative_difficulties, u N) {
	u tar=0;
	for (u i=1; i <= N; i++ ) {
		tar += 1e13/(cumulative_difficulties[i]-cumulative_difficulties[i-1]);
	}
	return N*1e13/tar;
}

u solvetime_without_exploits (std::vector<u>timestamps, u T) {
	u previous_timestamp(0), this_timestamp(0), ST, i;
	previous_timestamp = timestamps.front()-T;
	for ( i = 0; i < timestamps.size(); i++) {
 	 if ( timestamps[i] > previous_timestamp ) { this_timestamp = timestamps[i]; } 
 	 else { this_timestamp = previous_timestamp+1; }
	  ST = std::max(1+T/50,this_timestamp - previous_timestamp);
 	 previous_timestamp = this_timestamp; 
	}
	return ST;
}
// ===============================================
// =======  Simple Moving Average (SMA)	========
// ===============================================
u SMA_(std::vector<u> timestamps, 
	std::vector<u> cumulative_difficulties, u T, u N, u height,
					u FORK_HEIGHT,u difficulty_guess) {
	// If it's Genesis ...
	if (height <= N+1 ) { return difficulty_guess; }

	// Hard code D if there are not at least N+1 BLOCKS after fork (or genesis)
	if (height >= FORK_HEIGHT && height <= FORK_HEIGHT + N+1) { return difficulty_guess; }

	// Check to see if the timestamp and CD vectors are correct size.
	assert(timestamps.size() == N+1 && timestamps.size() == cumulative_difficulties.size()); 

	u ST = std::max(N,timestamps[N] - timestamps[0]);
	// Harmonic_mean of difficulties is the mean target and more accurate than mean difficulty.
	return harmonic_mean(cumulative_difficulties, N)* T *N/ ST;
}
// ===============================================
// =======  SMA with Slope Adjustment (SMS)	========
// ===============================================
// I thought this would be better than SMA, but it's not.  Maybe a little worse. I needed to do it with targets.
u SMS_(std::vector<u> timestamps, 
	std::vector<u> cumulative_difficulties, u T, u N, u height,
					u FORK_HEIGHT,u difficulty_guess) {
	if (height <= N+1 ) { return difficulty_guess; }
	if (height >= FORK_HEIGHT && height <= FORK_HEIGHT + N+1) { return difficulty_guess; }
	assert(timestamps.size() == N+1 && timestamps.size() == cumulative_difficulties.size()); 

	 int64_t slope = (cumulative_difficulties[N]-cumulative_difficulties[N/2])*T / (timestamps[N] - timestamps[N/2]) ;
	slope = slope - (cumulative_difficulties[N/2]-cumulative_difficulties[0])*T / (timestamps[N/2] - timestamps[0]) ;

	int64_t ST = std::max(N,timestamps[N] - timestamps[0]);
	int64_t CD = (cumulative_difficulties[N]-cumulative_difficulties[0]);
	return static_cast<u>(CD*T / ST + slope/2);
}
// ===========================
// ==========	DGW	========
// ===========================
// DGW_ uses an insanely complicated loop that is a simple moving average with double
// weight given to the most recent difficulty, which has virtually no effect.  So this 
// is just a SMA_ with the 1/3 and 3x limits in DGW_.
u DGW_(std::vector<u> timestamps, 
	std::vector<u> cumulative_difficulties, u T, u N, u height,
					u FORK_HEIGHT,u  difficulty_guess) {
	assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );
	if (height >= FORK_HEIGHT && height <= FORK_HEIGHT + N+1) { return difficulty_guess; }
	assert(timestamps.size() == N+1); 

	u ST = std::max(N,timestamps[N] - timestamps[0]);
  ST = std::max((N*T)/3,std::min((N*T)*3,ST));
	return u (cumulative_difficulties[N]-cumulative_difficulties[0])*T / ST;
}
// ================================
// ==========  Digishield  ========
// ================================
// Digishield uses the median of past 11 timestamps as the beginning and end of the window.
// This only simulates that, assuming the timestamps are always in the correct order.
// Also, this is in terms of difficulty instead of target, so a "101/100" correction factor was used.
u DIGISHIELD_(std::vector<u> timestamps, 
	std::vector<u> cumulative_difficulties, u T, u N, u height,
	u FORK_HEIGHT,u  difficulty_guess) {

	// Genesis should be the only time sizes are < N+1.
	assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );

	// Hard code D if there are not at least N+1 BLOCKS after fork (or genesis)
	if (height >= FORK_HEIGHT && height <= FORK_HEIGHT + N+1) { return difficulty_guess; }
	assert(timestamps.size() == N+1); 
	u tar=0;
	for (u i=6+1; i<=N; i++ ) {
		tar += 1e13/(cumulative_difficulties[i]-cumulative_difficulties[i-1]);
	}
	u harm_mean_diffs = (N-6)*1e13/tar;
	u ST = std::max(N,timestamps[N-6] - timestamps[0]);
	// ST = std::max((((N-6)*T)*84)/100,std::min(((N-6)*T*132)/100,(300*(N-6)*T+100*ST)/400));
	return  ((harm_mean_diffs*T*400)/(300*(N-6)*T+100*ST))*(N-6);
}
// ===========================================
// ==========	improved Digishield	(6-block timestamp delay removed)
// ===========================================
u DIGISHIELD_improved_(std::vector<u> timestamps, 
	std::vector<u> cumulative_difficulties, u T, u N, u height,
					u FORK_HEIGHT,u  difficulty_guess) {
	assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );

	if (height >= FORK_HEIGHT && height <= FORK_HEIGHT + N+1) { return difficulty_guess; }
	assert(timestamps.size() == N+1); 
	u ST = std::max(N,timestamps[N] - timestamps[0]);
	return ((harmonic_mean(cumulative_difficulties, N)*T*400)/(300*N*T+100*ST))*N;
}
// ==============================
// ==========	LWMA-1	========
// ==============================
// LWMA difficulty algorithm 
// Copyright (c) 2017-2018 Zawy, MIT License
// https://github.com/zawy12/difficulty-algorithms/issues/3
// See commented version for explanations & required config file changes. Fix FTL and MTP!
// REMOVE COMMENTS BELOW THIS LINE. 
// The options are recommended. They're called options to show the core is a simple LWMA.
// Bitcoin clones must lower their FTL. 
// Cryptonote et al coins must make the following changes:
// #define BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW	 11
// #define CRYPTONOTE_BLOCK_FUTURE_TIME_LIMIT		  3 * DIFFICULTY_TARGET 
// #define DIFFICULTY_WINDOW			90 //  N=60, 90, and 120 for T=600, 120, 60.
// Warning Bytecoin/Karbo clones may not have the following, so check TS & CD vectors size=N+1
// #define DIFFICULTY_BLOCKS_COUNT		 DIFFICULTY_WINDOW+1
// The BLOCKS_COUNT is to make timestamps & cumulative_difficulty vectors size N+1
// Do not sort timestamps.  
// CN coins (Monero < 12.3) must deploy the Jagerman MTP Patch. See:
// https://github.com/loki-project/loki/pull/26	or
// https://github.com/graft-project/GraftNetwork/pull/118/files

u LWMA1_(std::vector<u> timestamps, 
	std::vector<u> cumulative_difficulties, u T, u N, u height,  
					u FORK_HEIGHT,u  difficulty_guess) {
	// Genesis should be the only time sizes are < N+1.
	assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );

	// Hard code D if there are not at least N+1 BLOCKS after fork (or genesis)
	if (height >= FORK_HEIGHT && height <= FORK_HEIGHT + N+1) { return difficulty_guess; }
	assert(timestamps.size() == N+1); 

	u  L(0), next_D, i, this_timestamp(0), previous_timestamp(0), avg_D;
	
	previous_timestamp = timestamps[0];
	for ( i = 1; i <= N; i++) {		  
		// Safely prevent out-of-sequence timestamps
		if ( timestamps[i]  > previous_timestamp ) {	this_timestamp = timestamps[i];  } 
		else {  this_timestamp = previous_timestamp+1;	}
		L +=  i*std::min(6*T ,this_timestamp - previous_timestamp);
		previous_timestamp = this_timestamp; 
	}
	if (L < N*N*T/20 ) { L =  N*N*T/20; }
	avg_D = ( cumulative_difficulties[N] - cumulative_difficulties[0] )/ N;
	
	// Prevent round off error for small D and overflow for large D.
	if (avg_D > 2000000*N*N*T) { next_D = (avg_D/(200*L))*(N*(N+1)*T*99); }	
	else { next_D = (avg_D*N*(N+1)*T*99)/(200*L); }	
	return  next_D;
}
// ==============================
// ==========	LWMA-4	========
// ==============================
// LWMA-4 difficulty algorithm 
// Copyright (c) 2017-2018 Zawy, MIT License
// https://github.com/zawy12/difficulty-algorithms/issues/3
// See commented version for explanations & required config file changes. Fix FTL and MTP!

u LWMA4_(std::vector<u> timestamps, 
	std::vector<u> cumulative_difficulties, u T, u N, u height,  
					u FORK_HEIGHT,u  difficulty_guess) {
	// Genesis should be the only time sizes are < N+1.
	assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );
	// Hard code D if there are not at least N+1 BLOCKS after fork (or genesis)
	if (height >= FORK_HEIGHT && height <= FORK_HEIGHT + N+1) { return difficulty_guess; }
	assert(timestamps.size() == N+1); 

	u  L(0), ST(0), next_D, prev_D, avg_D, i;

	// Safely convert out-of-sequence timestamps into > 0 solvetimes.
	std::vector<u>TS(N+1);
	TS[0] = timestamps[0];
	for ( i = 1; i <= N; i++) {		  
		if ( timestamps[i]  > TS[i-1]  ) {	TS[i] = timestamps[i];  } 
		else {  TS[i] = TS[i-1];	}
	}

	for ( i = 1; i <= N; i++) {  
		// Temper long solvetime drops if they were preceded by 3 or 6 fast solves.
		if ( i > 4 && TS[i]-TS[i-1] > 5*T  && TS[i-1] - TS[i-4] < (14*T)/10 ) {	ST = 2*T; }
		else if ( i > 7 && TS[i]-TS[i-1] > 5*T  && TS[i-1] - TS[i-7] < 4*T ) {	ST = 2*T; }
		else { // Assume normal conditions, so get ST.
			// LWMA drops too much from long ST, so limit drops with a 5*T limit 
			ST = std::min(5*T ,TS[i] - TS[i-1]);
		}
		L +=  ST * i ; 
	} 
	if (L < N*N*T/20 ) { L =  N*N*T/20; } 
	avg_D = ( cumulative_difficulties[N] - cumulative_difficulties[0] )/ N;
	
	// Prevent round off error for small D and overflow for large D.
	if (avg_D > 2000000*N*N*T) { 
		 next_D = (avg_D/(200*L))*(N*(N+1)*T*97);	
	}	
	else {	 next_D = (avg_D*N*(N+1)*T*97)/(200*L);	 }

	prev_D =  cumulative_difficulties[N] - cumulative_difficulties[N-1] ; 

	// Apply 10% jump rule.
	if (  ( TS[N] - TS[N-1] < (2*T)/10 ) || 
			( TS[N] - TS[N-2] < (5*T)/10 ) ||  
			( TS[N] - TS[N-3] < (8*T)/10 )	 )
	{   next_D = std::max( next_D, std::min( (prev_D*110)/100, (105*avg_D)/100 ) );  }
	// Make all insignificant digits zero for easy reading.
	i = 1000000000;
	while (i > 1) { 
	  if ( next_D > i*100 ) { next_D = ((next_D+i/2)/i)*i; break; }
	  else { i /= 10; }
	}
	// Make least 3 digits equal avg of past 10 solvetimes.
	if ( next_D > 100000 ) { 
	 next_D = ((next_D+500)/1000)*1000 + std::min(static_cast<u>(999), (TS[N]-TS[N-10])/10); 
	}
	return  next_D;
}
// ============================
// ========  WHR	===========
// ============================
// This is a test to see if the original WT-144 of LWMA by Tom Harding that weighted
// hashrate (difficulty/solvetime) instead of solvetimes could be made as good as 
// or better than LWMA. Yes, it was as good, but not better.

u WHR_(std::vector<u> timestamps, 
	std::vector<u> cumulative_difficulties, u T, u N, u height,  
					u FORK_HEIGHT,u  difficulty_guess) {
	// Genesis should be the only time sizes are < N+1.
	assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );
	assert(N%2 == 0); 
	// Hard code D if there are not at least N+1 BLOCKS after fork (or genesis)
	if (height >= FORK_HEIGHT && height <= FORK_HEIGHT + N+1) { return difficulty_guess; }
	assert(timestamps.size() == N+1); 

	u  L(0), next_D, i, this_timestamp(0), previous_timestamp(0), avg_D, j(0);
	u ts =0, WHR, tar1, tar2;
//	if (height % 2) { // The if-else statement was not as accurate.
		previous_timestamp=timestamps[0];
		for ( i = 2; i <= N; i+=2) {	
			j++;
			// Safely prevent out-of-sequence timestamps
			if ( timestamps[i]  > previous_timestamp ) {	this_timestamp = timestamps[i];  } 
			else {  this_timestamp = previous_timestamp+1;	}
			ts = this_timestamp - previous_timestamp;
			previous_timestamp = this_timestamp; 
			tar1 = 1e13/(cumulative_difficulties[i] - cumulative_difficulties[i-1]);
			tar2 = 1e13/(cumulative_difficulties[i-1] - cumulative_difficulties[i-2]);
			WHR +=  (j*ts)/2 * (tar1 + tar2)/2 * 2/(2-1); // 1/2's are for ts & target avgs
		}
		next_D = ((1e13*T*j)*(j+1))/WHR;  
//	}
//	else {next_D = cumulative_difficulties[N] - cumulative_difficulties[N-1]; }
	return  next_D;
}
// ============================
// ========  LWMA_ASERT	===========
// ============================
// This is a test to see if the original WT-144 of LWMA by Tom Harding that weighted
// hashrate (difficulty/solvetime) instead of solvetimes could be made as good as 
// or better than LWMA. Yes, it was as good, but not better.

u LWMA_ASERT_(std::vector<u> timestamps, 
	std::vector<u> cumulative_difficulties, u T, u N, u height,  
					u FORK_HEIGHT,u  difficulty_guess, u M) {
	// Genesis should be the only time sizes are < N+1.
	assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );
	assert(N%2 == 0); 
	// Hard code D if there are not at least N+1 BLOCKS after fork (or genesis)
	if (height >= FORK_HEIGHT && height <= FORK_HEIGHT + N+1) { return difficulty_guess; }
	assert(timestamps.size() == N+1); 

	u  L(0), next_D, i, this_timestamp(0), previous_timestamp(0), avg_D;
	u ST =0, WHR, tar1, exp_B, exp_A;
		previous_timestamp=timestamps[0];
		for ( i = 1; i <= N; i++) {		  
			// Safely prevent out-of-sequence timestamps
			if ( timestamps[i]  > previous_timestamp ) {	this_timestamp = timestamps[i];  } 
			else {  this_timestamp = previous_timestamp+1;	}
			ST = this_timestamp - previous_timestamp;
			previous_timestamp = this_timestamp; 
			tar1 = 1e13/(cumulative_difficulties[i] - cumulative_difficulties[i-1]);
			exp_B = exponential_function_for_integers((ST*1E6)/T/M);
			exp_A = exponential_function_for_integers(1E6/M);
			WHR += (i*tar1)/exp_A*exp_B;
		}
		next_D = N/2*(N+1)/WHR;  
	return  next_D;
}
// ============================
// ========	TSA	===========
// ============================

u TSA_(std::vector<u> timestamps, std::vector<u> cumulative_difficulties, u T, u N, u height,  
				u FORK_HEIGHT, u  difficulty_guess, u template_timestamp, u M ) {
	assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );
	if (height >= FORK_HEIGHT && height <= FORK_HEIGHT + N+1) { return difficulty_guess; }
	assert(timestamps.size() == N+1); 

	u ST = timestamps[N] - timestamps[N-1];
	u prev_D = cumulative_difficulties[N] - cumulative_difficulties[N-1];
	u next_D = (prev_D*N*10000)/(10000*N+10000*ST/T-10000);
	
	ST = template_timestamp - timestamps[N];
	u TSA_D = (next_D*M*10000)/(10000*M+10000*ST/T-10000);

	return TSA_D; 	
}
// ============================
// ========  Boris (Zcoin) aka KGW	
// ============================
/* Boris, aka BRNDF in Zcoin and Monocle, is Kimoto Gravity Well (KGW)  with doubles removed.
KGW is originally from Megacoin. It was developed by Dr Kimoto Chan.  Those that use KGW:
MaxCoin, Reddcoin, CasinoCoin, Klondikecoin, Franko, AnonCoin, MemoryCoin, RonPaulCoin(?)
The "Fast" values below come from ratio = 1+0.7084*(N/144)^(-1.228) where N is the size of the 
averaging window. The slow values are the inverse.  Ostensibly this SMA averaging window
is 7*144 blocks, but you quit the loop early with a smaller averaging window if the blocks 
come too fast or too slow by ratio or 1/ratio. The equation is only 
good for 144, but it does not need to change if target solvetime changes.
*/ 
u Boris_(std::vector<u> timestamps, 
	std::vector<u> cumulative_difficulties, u uT, u uN, u height,  
					u FORK_HEIGHT,u  difficulty_guess) {
float SlowBlocksLimit[1050] = {0.00315,0.00734,0.0120,0.0170,0.0223,0.0277,0.0333,0.0380,0.0447,0.0507,0.0566,0.0626,0.0686,0.0746,0.0807,0.0868,0.0929,0.0980,0.105,0.111,0.117,0.123,0.129,0.135,0.141,0.147,0.153,0.158,0.164,0.170,0.176,0.182,0.187,0.193,0.199,0.204,0.210,0.215,0.221,0.226,0.231,0.237,0.242,0.247,0.252,0.257,0.263,0.268,0.273,0.278,0.282,0.287,0.292,0.297,0.302,0.306,0.311,0.316,0.320,0.325,0.329,0.334,0.338,0.342,0.347,0.351,0.355,0.359,0.363,0.367,0.372,0.376,0.380,0.383,0.387,0.391,0.395,0.399,0.403,0.406,0.410,0.414,0.417,0.421,0.424,0.428,0.431,0.435,0.438,0.442,0.445,0.448,0.452,0.455,0.458,0.461,0.464,0.468,0.471,0.474,0.477,0.480,0.483,0.486,0.489,0.492,0.495,0.497,0.500,0.503,0.506,0.509,0.511,0.514,0.517,0.519,0.522,0.525,0.527,0.530,0.532,0.535,0.537,0.540,0.542,0.545,0.547,0.549,0.552,0.554,0.556,0.559,0.561,0.563,0.565,0.568,0.570,0.572,0.574,0.576,0.579,0.581,0.583,0.585,0.587,0.589,0.591,0.593,0.595,0.597,0.599,0.601,0.603,0.605,0.607,0.608,0.610,0.612,0.614,0.616,0.618,0.619,0.621,0.623,0.625,0.626,0.628,0.630,0.632,0.633,0.635,0.637,0.638,0.640,0.642,0.643,0.645,0.646,0.648,0.649,0.651,0.653,0.654,0.656,0.657,0.659,0.660,0.661,0.663,0.664,0.666,0.667,0.669,0.670,0.671,0.673,0.674,0.676,0.677,0.678,0.680,0.681,0.682,0.684,0.685,0.686,0.687,0.689,0.690,0.691,0.692,0.694,0.695,0.696,0.697,0.699,0.700,0.701,0.702,0.703,0.704,0.706,0.707,0.708,0.709,0.710,0.711,0.712,0.713,0.714,0.716,0.717,0.718,0.719,0.720,0.721,0.722,0.723,0.724,0.725,0.726,0.727,0.728,0.729,0.730,0.731,0.732,0.733,0.734,0.735,0.736,0.737,0.738,0.739,0.740,0.741,0.741,0.742,0.743,0.744,0.745,0.746,0.747,0.748,0.749,0.749,0.750,0.751,0.752,0.753,0.754,0.755,0.755,0.756,0.757,0.758,0.759,0.759,0.760,0.761,0.762,0.763,0.763,0.764,0.765,0.766,0.767,0.767,0.768,0.769,0.770,0.770,0.771,0.772,0.773,0.773,0.774,0.775,0.775,0.776,0.777,0.778,0.778,0.779,0.780,0.780,0.781,0.782,0.782,0.783,0.784,0.784,0.785,0.786,0.786,0.787,0.788,0.788,0.789,0.790,0.790,0.791,0.791,0.792,0.793,0.793,0.794,0.795,0.795,0.796,0.796,0.797,0.798,0.798,0.799,0.799,0.800,0.800,0.801,0.802,0.802,0.803,0.803,0.804,0.804,0.805,0.806,0.806,0.807,0.807,0.808,0.808,0.809,0.809,0.810,0.810,0.811,0.812,0.812,0.813,0.813,0.814,0.814,0.815,0.815,0.816,0.816,0.817,0.817,0.818,0.818,0.819,0.819,0.820,0.820,0.821,0.821,0.821,0.822,0.822,0.823,0.823,0.824,0.824,0.825,0.825,0.826,0.826,0.827,0.827,0.827,0.828,0.828,0.829,0.829,0.830,0.830,0.831,0.831,0.831,0.832,0.832,0.833,0.833,0.834,0.834,0.834,0.835,0.835,0.836,0.836,0.836,0.837,0.837,0.838,0.838,0.838,0.839,0.839,0.840,0.840,0.840,0.841,0.841,0.842,0.842,0.842,0.843,0.843,0.843,0.844,0.844,0.845,0.845,0.845,0.846,0.846,0.846,0.847,0.847,0.848,0.848,0.848,0.849,0.849,0.849,0.850,0.850,0.850,0.851,0.851,0.851,0.852,0.852,0.852,0.853,0.853,0.853,0.854,0.854,0.854,0.855,0.855,0.855,0.856,0.856,0.856,0.857,0.857,0.857,0.858,0.858,0.858,0.859,0.859,0.859,0.860,0.860,0.860,0.860,0.861,0.861,0.861,0.862,0.862,0.862,0.863,0.863,0.863,0.863,0.864,0.864,0.864,0.865,0.865,0.865,0.865,0.866,0.866,0.866,0.867,0.867,0.867,0.867,0.868,0.868,0.868,0.869,0.869,0.869,0.869,0.870,0.870,0.870,0.870,0.871,0.871,0.871,0.872,0.872,0.872,0.872,0.873,0.873,0.873,0.873,0.874,0.874,0.874,0.874,0.875,0.875,0.875,0.875,0.876,0.876,0.876,0.876,0.877,0.877,0.877,0.877,0.878,0.878,0.878,0.878,0.879,0.879,0.879,0.879,0.880,0.880,0.880,0.880,0.880,0.881,0.881,0.881,0.881,0.882,0.882,0.882,0.882,0.883,0.883,0.883,0.883,0.883,0.884,0.884,0.884,0.884,0.885,0.885,0.885,0.885,0.885,0.886,0.886,0.886,0.886,0.886,0.887,0.887,0.887,0.887,0.887,0.888,0.888,0.888,0.888,0.889,0.889,0.889,0.889,0.889,0.890,0.890,0.890,0.890,0.890,0.891,0.891,0.891,0.891,0.891,0.892,0.892,0.892,0.892,0.892,0.892,0.893,0.893,0.893,0.893,0.893,0.894,0.894,0.894,0.894,0.894,0.895,0.895,0.895,0.895,0.895,0.895,0.896,0.896,0.896,0.896,0.896,0.897,0.897,0.897,0.897,0.897,0.897,0.898,0.898,0.898,0.898,0.898,0.898,0.899,0.899,0.899,0.899,0.899,0.900,0.900,0.900,0.900,0.900,0.900,0.901,0.901,0.901,0.901,0.901,0.901,0.902,0.902,0.902,0.902,0.902,0.902,0.902,0.903,0.903,0.903,0.903,0.903,0.903,0.904,0.904,0.904,0.904,0.904,0.904,0.905,0.905,0.905,0.905,0.905,0.905,0.905,0.906,0.906,0.906,0.906,0.906,0.906,0.907,0.907,0.907,0.907,0.907,0.907,0.907,0.908,0.908,0.908,0.908,0.908,0.908,0.908,0.909,0.909,0.909,0.909,0.909,0.909,0.909,0.910,0.910,0.910,0.910,0.910,0.910,0.910,0.911,0.911,0.911,0.911,0.911,0.911,0.911,0.911,0.912,0.912,0.912,0.912,0.912,0.912,0.912,0.913,0.913,0.913,0.913,0.913,0.913,0.913,0.913,0.914,0.914,0.914,0.914,0.914,0.914,0.914,0.914,0.915,0.915,0.915,0.915,0.915,0.915,0.915,0.915,0.916,0.916,0.916,0.916,0.916,0.916,0.916,0.916,0.917,0.917,0.917,0.917,0.917,0.917,0.917,0.917,0.918,0.918,0.918,0.918,0.918,0.918,0.918,0.918,0.919,0.919,0.919,0.919,0.919,0.919,0.919,0.919,0.919,0.920,0.920,0.920,0.920,0.920,0.920,0.920,0.920,0.920,0.921,0.921,0.921,0.921,0.921,0.921,0.921,0.921,0.921,0.922,0.922,0.922,0.922,0.922,0.922,0.922,0.922,0.922,0.923,0.923,0.923,0.923,0.923,0.923,0.923,0.923,0.923,0.923,0.924,0.924,0.924,0.924,0.924,0.924,0.924,0.924,0.924,0.924,0.925,0.925,0.925,0.925,0.925,0.925,0.925,0.925,0.925,0.925,0.926,0.926,0.926,0.926,0.926,0.926,0.926,0.926,0.926,0.926,0.927,0.927,0.927,0.927,0.927,0.927,0.927,0.927,0.927,0.927,0.927,0.928,0.928,0.928,0.928,0.928,0.928,0.928,0.928,0.928,0.928,0.929,0.929,0.929,0.929,0.929,0.929,0.929,0.929,0.929,0.929,0.929,0.930,0.930,0.930,0.930,0.930,0.930,0.930,0.930,0.930,0.930,0.930,0.930,0.931,0.931,0.931,0.931,0.931,0.931,0.931,0.931,0.931,0.931,0.931,0.932,0.932,0.932,0.932,0.932,0.932,0.932,0.932,0.932,0.932,0.932,0.932,0.933,0.933,0.933,0.933,0.933,0.933,0.933,0.933,0.933,0.933,0.933,0.933,0.934,0.934,0.934,0.934,0.934,0.934,0.934,0.934,0.934,0.934,0.934,0.934,0.934,0.935,0.935,0.935,0.935,0.935,0.935,0.935,0.935,0.935,0.935,0.935,0.935,0.935,0.936,0.936,0.936,0.936,0.936,0.936,0.936,0.936,0.936,0.936,0.936,0.936,0.936,0.937,0.937,0.937,0.937,0.937,0.937,0.937,0.937,0.937,0.937,0.937,0.937,0.937,0.937,0.938,0.938,0.938,0.938,0.938,0.938,0.938,0.938,0.938,0.938,0.938,0.938,0.938,0.938,0.939,0.939,0.939,0.939,0.939,0.939,0.939,0.939,0.939};
float FastBlocksLimit[1050] = {317.772,136.233,83.194,58.732,44.894,36.089,30.038,25.646,22.327,19.739,17.669,15.980,14.577,13.396,12.389, 11.521,10.766,10.104,9.519,8.999,8.534,8.116,7.738,7.395,7.082,6.796,6.533,6.292,6.069,5.862,5.670,5.491,5.325,5.169,5.023,4.886,4.758,4.637,4.523,4.415,4.313,4.216,4.124,4.038,3.955,3.876,3.801,3.730,3.661,3.596,3.534,3.474,3.417,3.362,3.309,3.259,3.210,3.164,3.119,3.075,3.034,2.993,2.955,2.917,2.881,2.846,2.812,2.780,2.748,2.717,2.688,2.659,2.631,2.604,2.578,2.552,2.528,2.504,2.480,2.457,2.435,2.414,2.393,2.373,2.353,2.334,2.315,2.296,2.279,2.261,2.244,2.228,2.211,2.196,2.180,2.165,2.150,2.136,2.122,2.108,2.095,2.081,2.069,2.056,2.044,2.031,2.020,2.008,1.997,1.986,1.975,1.964,1.954,1.943,1.933,1.923,1.914,1.904,1.895,1.886,1.877,1.868,1.859,1.851,1.842,1.834,1.826,1.818,1.810,1.803,1.795,1.788,1.781,1.773,1.766,1.759,1.753,1.746,1.739,1.733,1.726,1.720,1.714,1.708,1.702,1.696,1.690,1.684,1.679,1.673,1.668,1.662,1.657,1.652,1.647,1.642,1.637,1.632,1.627,1.622,1.617,1.613,1.608,1.603,1.599,1.594,1.590,1.586,1.581,1.577,1.573,1.569,1.565,1.561,1.557,1.553,1.549,1.546,1.542,1.538,1.534,1.531,1.527,1.524,1.520,1.517,1.513,1.510,1.507,1.504,1.500,1.497,1.494,1.491,1.488,1.485,1.482,1.479,1.476,1.473,1.470,1.467,1.464,1.461,1.459,1.456,1.453,1.450,1.448,1.445,1.443,1.440,1.438,1.435,1.433,1.430,1.428,1.425,1.423,1.420,1.418,1.416,1.414,1.411,1.409,1.407,1.405,1.402,1.400,1.398,1.396,1.394,1.392,1.390,1.388,1.386,1.384,1.382,1.380,1.378,1.376,1.374,1.372,1.370,1.368,1.367,1.365,1.363,1.361,1.359,1.358,1.356,1.354,1.352,1.351,1.349,1.347,1.346,1.344,1.342,1.341,1.339,1.338,1.336,1.334,1.333,1.331,1.330,1.328,1.327,1.325,1.324,1.322,1.321,1.320,1.318,1.317,1.315,1.314,1.313,1.311,1.310,1.308,1.307,1.306,1.305,1.303,1.302,1.301,1.299,1.298,1.297,1.296,1.294,1.293,1.292,1.291,1.290,1.288,1.287,1.286,1.285,1.284,1.282,1.281,1.280,1.279,1.278,1.277,1.276,1.275,1.274,1.273,1.271,1.270,1.269,1.268,1.267,1.266,1.265,1.264,1.263,1.262,1.261,1.260,1.259,1.258,1.257,1.256,1.255,1.254,1.253,1.253,1.252,1.251,1.250,1.249,1.248,1.247,1.246,1.245,1.244,1.244,1.243,1.242,1.241,1.240,1.239,1.238,1.238,1.237,1.236,1.235,1.234,1.233,1.233,1.232,1.231,1.230,1.229,1.229,1.228,1.227,1.226,1.226,1.225,1.224,1.223,1.223,1.222,1.221,1.220,1.220,1.219,1.218,1.217,1.217,1.216,1.215,1.215,1.214,1.213,1.213,1.212,1.211,1.211,1.210,1.209,1.209,1.208,1.207,1.207,1.206,1.205,1.205,1.204,1.203,1.203,1.202,1.202,1.201,1.200,1.200,1.199,1.198,1.198,1.197,1.197,1.196,1.195,1.195,1.194,1.194,1.193,1.193,1.192,1.191,1.191,1.190,1.190,1.189,1.189,1.188,1.188,1.187,1.186,1.186,1.185,1.185,1.184,1.184,1.183,1.183,1.182,1.182,1.181,1.181,1.180,1.180,1.179,1.179,1.178,1.178,1.177,1.177,1.176,1.176,1.175,1.175,1.174,1.174,1.173,1.173,1.172,1.172,1.172,1.171,1.171,1.170,1.170,1.169,1.169,1.168,1.168,1.167,1.167,1.167,1.166,1.166,1.165,1.165,1.164,1.164,1.164,1.163,1.163,1.162,1.162,1.161,1.161,1.161,1.160,1.160,1.159,1.159,1.159,1.158,1.158,1.157,1.157,1.157,1.156,1.156,1.155,1.155,1.155,1.154,1.154,1.153,1.153,1.153,1.152,1.152,1.152,1.151,1.151,1.151,1.150,1.150,1.149,1.149,1.149,1.148,1.148,1.148,1.147,1.147,1.147,1.146,1.146,1.146,1.145,1.145,1.145,1.144,1.144,1.144,1.143,1.143,1.143,1.142,1.142,1.142,1.141,1.141,1.141,1.140,1.140,1.140,1.139,1.139,1.139,1.138,1.138,1.138,1.137,1.137,1.137,1.136,1.136,1.136,1.136,1.135,1.135,1.135,1.134,1.134,1.134,1.133,1.133,1.133,1.133,1.132,1.132,1.132,1.131,1.131,1.131,1.131,1.130,1.130,1.130,1.129,1.129,1.129,1.129,1.128,1.128,1.128,1.128,1.127,1.127,1.127,1.126,1.126,1.126,1.126,1.125,1.125,1.125,1.125,1.124,1.124,1.124,1.124,1.123,1.123,1.123,1.123,1.122,1.122,1.122,1.122,1.121,1.121,1.121,1.121,1.120,1.120,1.120,1.120,1.119,1.119,1.119,1.119,1.118,1.118,1.118,1.118,1.117,1.117,1.117,1.117,1.117,1.116,1.116,1.116,1.116,1.115,1.115,1.115,1.115,1.114,1.114,1.114,1.114,1.114,1.113,1.113,1.113,1.113,1.113,1.112,1.112,1.112,1.112,1.111,1.111,1.111,1.111,1.111,1.110,1.110,1.110,1.110,1.110,1.109,1.109,1.109,1.109,1.109,1.108,1.108,1.108,1.108,1.108,1.107,1.107,1.107,1.107,1.107,1.106,1.106,1.106,1.106,1.106,1.105,1.105,1.105,1.105,1.105,1.104,1.104,1.104,1.104,1.104,1.103,1.103,1.103,1.103,1.103,1.103,1.102,1.102,1.102,1.102,1.102,1.101,1.101,1.101,1.101,1.101,1.101,1.100,1.100,1.100,1.100,1.100,1.100,1.099,1.099,1.099,1.099,1.099,1.099,1.098,1.098,1.098,1.098,1.098,1.097,1.097,1.097,1.097,1.097,1.097,1.097,1.096,1.096,1.096,1.096,1.096,1.096,1.095,1.095,1.095,1.095,1.095,1.095,1.094,1.094,1.094,1.094,1.094,1.094,1.093,1.093,1.093,1.093,1.093,1.093,1.093,1.092,1.092,1.092,1.092,1.092,1.092,1.092,1.091,1.091,1.091,1.091,1.091,1.091,1.090,1.090,1.090,1.090,1.090,1.090,1.090,1.089,1.089,1.089,1.089,1.089,1.089,1.089,1.088,1.088,1.088,1.088,1.088,1.088,1.088,1.088,1.087,1.087,1.087,1.087,1.087,1.087,1.087,1.086,1.086,1.086,1.086,1.086,1.086,1.086,1.085,1.085,1.085,1.085,1.085,1.085,1.085,1.085,1.084,1.084,1.084,1.084,1.084,1.084,1.084,1.084,1.083,1.083,1.083,1.083,1.083,1.083,1.083,1.083,1.082,1.082,1.082,1.082,1.082,1.082,1.082,1.082,1.081,1.081,1.081,1.081,1.081,1.081,1.081,1.081,1.080,1.080,1.080,1.080,1.080,1.080,1.080,1.080,1.080,1.079,1.079,1.079,1.079,1.079,1.079,1.079,1.079,1.079,1.078,1.078,1.078,1.078,1.078,1.078,1.078,1.078,1.078,1.077,1.077,1.077,1.077,1.077,1.077,1.077,1.077,1.077,1.076,1.076,1.076,1.076,1.076,1.076,1.076,1.076,1.076,1.075,1.075,1.075,1.075,1.075,1.075,1.075,1.075,1.075,1.075,1.074,1.074,1.074,1.074,1.074,1.074,1.074,1.074,1.074,1.074,1.073,1.073,1.073,1.073,1.073,1.073,1.073,1.073,1.073,1.073,1.072,1.072,1.072,1.072,1.072,1.072,1.072,1.072,1.072,1.072,1.071,1.071,1.071,1.071,1.071,1.071,1.071,1.071,1.071,1.071,1.071,1.070,1.070,1.070,1.070,1.070,1.070,1.070,1.070,1.070,1.070,1.070,1.069,1.069,1.069,1.069,1.069,1.069,1.069,1.069,1.069,1.069,1.069,1.068,1.068,1.068,1.068,1.068,1.068,1.068,1.068,1.068,1.068,1.068,1.067,1.067,1.067,1.067,1.067,1.067,1.067,1.067,1.067,1.067,1.067,1.067,1.066,1.066,1.066,1.066,1.066,1.066,1.066,1.066,1.066,1.066,1.066,1.066,1.065,1.065,1.065,1.065,1.065,1.065,1.065,1.065,1.065,1.065,1.065,1.065,1.065,1.064,1.064,1.064,1.064,1.064,1.064,1.064};
		
	int64_t  L(0), next_D, i, N=uN, T=uT;
	int64_t target=0, tarPrev=0, tarAvg=0, j=0, ActualTimespan=0, TargetTimespan=0;
	double BTRatio;

	// Genesis should be the only time sizes are < N+1.
	assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );

	// Hard code D if there are not at least N+1 BLOCKS after fork (or genesis)
	if (height >= FORK_HEIGHT && height <= FORK_HEIGHT + N+1) { return difficulty_guess; }
	assert(timestamps.size() == N+1); 
 
	if (height % 1 == 0) { // change difficulty only once per 12 blocks
		for ( i = N; i >= 1; i--) {  // most recent block is N
			j++; // goes from 1 to N

			// Convert difficulty to target. *********  Not acceptable for live code.  ********
			target = 1e14/(cumulative_difficulties[i] - cumulative_difficulties[i-1]);
			
			// Boris's avg was complicated. This is simpler and the same thing
			tarPrev += target;  
			tarAvg = tarPrev/j;
			
			// If blocks are very fast or very slow, break out (don't do the full window, giving faster response)
			ActualTimespan = timestamps[N] - timestamps[i-1];
			TargetTimespan = T*j;
			if (j > 36) {
				BTRatio = double(TargetTimespan)/ActualTimespan;
				if ( BTRatio <= SlowBlocksLimit[j-1] || BTRatio >= FastBlocksLimit[j-1]) { break;}
			}
		}
		 tarAvg = std::min(2*tarAvg, std::max(tarAvg/2, tarAvg));
		ActualTimespan = std::min(3*ActualTimespan, std::max(ActualTimespan/3,ActualTimespan));

		next_D = 1e14*TargetTimespan/tarAvg/ActualTimespan;
	}
	else {next_D = cumulative_difficulties[N] - cumulative_difficulties[N-1]; }
	return next_D;
}
// ==============================
// ==========	EMA	===========  Tom's basic form with forced sequential timestamps
// ==============================
u EMA_(std::vector<u> timestamps, 
	std::vector<u> cumulative_difficulties, u T, u N, u height,
					u FORK_HEIGHT, u  difficulty_guess) {
	// Genesis should be the only time sizes are < N+1.
	assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );

	// Hard code D if there are not at least N+1 BLOCKS after fork (or genesis)
	if (height >= FORK_HEIGHT && height <= FORK_HEIGHT + N+1) { return difficulty_guess; }
	assert(timestamps.size() == N+1); 
		
	u previous_timestamp(0), this_timestamp(0), ST;
	 // Safely handle out-of-sequence timestamps
	// This is awkward because N defines a window of timestamps to review while EMA uses only previous timestamp
	previous_timestamp = 0;
	for (int i = 0; i < timestamps.size(); i++) {	// changing 0 to 1 causes problem??
		if ( timestamps[i] <= previous_timestamp ) { this_timestamp = previous_timestamp; } 
		else { this_timestamp = timestamps[i]; }
		ST = this_timestamp - previous_timestamp; 
		previous_timestamp = this_timestamp; 
	}
	 ST = (timestamps[N] - timestamps[N-1]);
	u prev_D = (cumulative_difficulties[N] - cumulative_difficulties[N-1]);
	return (prev_D*N*10000)/(10000*N+10000*ST/T-10000);	
}
// ==============================
// ==========	EMA3 (JE accurate)  Most precise version that does not have problems
// ==============================	other than potential overflow which is easier to fix in targets.
u EMA3_(std::vector<u> timestamps, 
	std::vector<u> cumulative_difficulties, u T, u N, u height,
					u FORK_HEIGHT,u  difficulty_guess) {
	// Genesis should be the only time sizes are < N+1.
	assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );

	// Hard code D if there are not at least N+1 BLOCKS after fork (or genesis)
	if (height >= FORK_HEIGHT && height <= FORK_HEIGHT + N+1) { return difficulty_guess; }
	assert(timestamps.size() == N+1); 
	
	//  (1+1/N+k*(k/2-1-1/2/N))
	//  (2*T*N*T*N+2*T*T*N+t*(t-2*T*N-T))/(N*T*N*T*2)
	u ST = timestamps[N] - timestamps[N-1];
	u prev_D = cumulative_difficulties[N] - cumulative_difficulties[N-1];
	return (prev_D *(2*T*N*T*N+2*T*T*N+ST*(ST-2*T*N-T)))/(N*T*N*T*2); // JE accurate
}
// =============================== This is the EMA perfected. Due to the way my exp() is used,
// ========  ASERT  ============== it does not have the error in usage that EMA did unles N>400 ish
// ===============================
u ASERT_(std::vector<u> timestamps, 
	std::vector<u> cumulative_difficulties, u T, u N, u height,
					u FORK_HEIGHT,u  difficulty_guess, u M) {
	// Genesis should be the only time sizes are < N+1.
	assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );

	// Hard code D if there are not at least N+1 BLOCKS after fork (or genesis)
	if (height >= FORK_HEIGHT && height <= FORK_HEIGHT + N+1) { return difficulty_guess; }
	assert(timestamps.size() == N+1); 

	u ST = timestamps[N] - timestamps[N-1] ;
	u exp_A = exponential_function_for_integers((1E6)/M);
	u exp_B = exponential_function_for_integers((ST*1E6)/M/T);
	return u (cumulative_difficulties[N]-cumulative_difficulties[N-1])*exp_A/exp_B;
}
// =============================== 
// ========  ASERT_SMA  ==========
// ===============================
u ASERT_SMA_(std::vector<u> timestamps, 
	std::vector<u> cumulative_difficulties, u T, u N, u height,
					u FORK_HEIGHT,u  difficulty_guess, u M) {
	// Genesis should be the only time sizes are < N+1.
	assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );

	// Hard code D if there are not at least N+1 BLOCKS after fork (or genesis)
	if (height >= FORK_HEIGHT && height <= FORK_HEIGHT + N+1) { return difficulty_guess; }
	assert(timestamps.size() == N+1); 
	
	u ST = (timestamps[N] - START_TIMESTAMP -(height-FORK_HEIGHT+2)*T) ;
	if (timestamps[N] < START_TIMESTAMP + (height-FORK_HEIGHT+2)*T) {
		ST = START_TIMESTAMP + (height-FORK_HEIGHT+2)*T - timestamps[N];
	}
	u exp_A = exponential_function_for_integers(1e6/M);
	u exp_B = exponential_function_for_integers((ST*1E6)/T/M);
	u tar=0;
	/*
	for (u i=1; i<=N;i++) {
		tar += 1e13/(cumulative_difficulties[i]-cumulative_difficulties[i-1]);
	}
	*/
	// tar = 1e13/(cumulative_difficulties[N-M]-cumulative_difficulties[N-M-1]);
	// return u (cumulative_difficulties[N]-cumulative_difficulties[0])/N*exp_A/exp_B;
	if (timestamps[N] < START_TIMESTAMP + (height-FORK_HEIGHT+2)*T) {
		return BASELINE_D*exp_A*exp_B/10000/10000;
	}
	else {	return BASELINE_D*exp_A/exp_B;	}
}
// =============================
// ========  ETH  ==============
// =============================
u ETH_(std::vector<u> timestamps,	 std::vector<u> cumulative_difficulties, u T, u N, u height,
					u FORK_HEIGHT,u  difficulty_guess) {
	// Genesis should be the only time sizes are < N+1.
	assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );

	// Hard code D if there are not at least N+1 BLOCKS after fork (or genesis)
	if (height >= FORK_HEIGHT && height <= FORK_HEIGHT + N+1) { return difficulty_guess; }
	assert(timestamps.size() == N+1); 
	
	u previous_timestamp(0), this_timestamp(0), ST;
	 // Safely handle out-of-sequence timestamps
	// This is awkward because N defines a window of timestamps to review but 
	// EMA only uses previous timestamp
	previous_timestamp = timestamps.front()- T;
	for ( int i = 0; i < timestamps.size(); i++) {
 		if ( timestamps[i] <= previous_timestamp ) { this_timestamp = previous_timestamp+1; } 
 		else { this_timestamp = timestamps[i]; }
		ST = std::max(static_cast<u>(1),this_timestamp - previous_timestamp);
 		previous_timestamp = this_timestamp; 
	}
	return u ((cumulative_difficulties[N]-cumulative_difficulties[N-1])*N/(N+(1443*ST/T)/1000-1)); // 1443/1000 = 1/ln(2)
}
// ==============================================
//  =========	RUN SIMULATION  =================
// ==============================================

int run_simulation(string DA, u T, u N,u difficulty_guess,u baseline_HR,u attack_start,u attack_stop,u attack_size, u R) {


cout << DA << " blocks to simulate: " << BLOCKS << ". Baseline Diff: " << BASELINE_D << ". Target timespan: " << T << 
". Start/stop (attacks size) as multiples of baseline difficulty (hashrate): " << float(attack_start)/100 << " / " << 
float(attack_stop)/100 << " (" << float(attack_size)/100 << ")" << endl;

// R is used for EMA and TSA

if ( DA == "DIGISHIELD_" ) { N=N+6; } // For simulated MTP = 11 delay.

	// In theory, this just uses ST = -ln(rand())* T * D/HR, but it's a mess, half-way because of CN_delay.
	// I should have made two different methods.  One with CN delay and one without.
	// Even without that it would have been a mess because of genesis or fork option.  Needed 4 methods.

	u next_D(0), TSA_D, simulated_ST(0), i, HR(0), attack_on(0);
	u previous_ST, current_ST;
	vector<u>STs(BLOCKS);
	vector<u>Ds(BLOCKS);
	vector<u>HRs(BLOCKS);
	vector<float>Dtsa(BLOCKS,0); // TSA will be the only 1 to change the all-0 values

	// Initially assume it's genesis
	vector<u>TS(1);
	TS[0] = START_TIMESTAMP;

	vector<u>CD(1);
	CD[0] = START_CD; 

	// If it's a fork initialize (make up) previous N BLOCKS
	
	if (FORK_HEIGHT >= N+1) {
		TS.resize(N+1);
		CD.resize(N+1);
		// Note that TS[N] is not stored. CD must be 1 block ahead because
		// TS[N] depends on CD[N] (unless CN delay) in which case TS[N+1] depends on CD[N].
		for (i=1; i<=N; i++) {	
			TS[i] = TS[i-1]+T;
			CD[i] = CD[i-1]+BASELINE_D; 
		} 
	}
	else if (FORK_HEIGHT == 0 ) { 
		TS.push_back(TS[0]+T); 
		CD.push_back(CD[0]+BASELINE_D);
	}
	else if (FORK_HEIGHT > 1 ) {
		cout << "This is not the reason the program crashed, but you can't fork if genesis was < N+1 BLOCKS in the past." << endl; 
	}
// *****  Run simulation  ********
previous_ST = T; // initialize in case we are using CN delay.
HR = baseline_HR;
u height = 0; 

float mN = static_cast<float>(N);
float mT = static_cast<float>(T);

float avgST(0), avgHR(0), avgD(0), avgDtsa(0);

for (i=0; i <= BLOCKS-1; i++) {
	// attack_size = 100 means there's no hash attack, but just constant HR.
	if (next_D > (attack_stop*BASELINE_D)/100 ) {  attack_on = 0;  HR = baseline_HR;	  }
	else if (next_D < (attack_start*BASELINE_D)/100 ) { attack_on = 1; HR = (baseline_HR*attack_size)/100; }

	if (DA == "ASERT_RTT_") {	
		if ( i == 0 ) {	//  prevents assert error since this is different from other DAs.
			CD.push_back(CD.back()+BASELINE_D); 
			Ds.push_back(BASELINE_D);
			HRs.push_back(baseline_HR);
		}
		// My old method that Mark Lundeberg showed me was wrong
		// current_ST = static_cast<u>(0.5+mT*mN/2*(pow(1+NEG_LOG_RAND[i]*4*pow(2.7182,1/mN)*static_cast<float>((CD.back()-CD[CD.size()-2])*DX/HR)/mT/mN,0.5) - 1));
		// Now implementing t=T*N*ln(1-e^(1/N)*D/(HR*T*N)*ln(x))
		current_ST = static_cast<u>(0.5+mT*mN*log(1+pow(2.7182,1/mN)*NEG_LOG_RAND[i]
		*static_cast<float>((CD.back()-CD[CD.size()-2])*DX/HR)/mT/mN));
		simulated_ST = current_ST;
		cout << NEG_LOG_RAND[i] << " xxx " << current_ST << endl;
		TS.push_back(TS.back() + current_ST); 
		if (TS.size() > N+1 ) { TS.erase(TS.begin()); }
	}
  // if (HR == 0) { HR=1; cout << "HR was zero, so it was changed to 1 to prevent 1/0." << endl; }
  // Run DA
  
	height = FORK_HEIGHT+i;

		if (DA == "LWMA1_" ) {	next_D = LWMA1_(TS,CD,T,N, height, FORK_HEIGHT, difficulty_guess);  }
		if (DA == "LWMA4_" ) {	next_D = LWMA4_(TS,CD,T,N,height,FORK_HEIGHT, difficulty_guess);  }
		if (DA == "WHR_" ) {	next_D = WHR_(TS,CD,T,N, height, FORK_HEIGHT, difficulty_guess);  }
		if (DA == "KGW_" || DA == "Boris_" ) {	next_D = Boris_(TS,CD,T,N,height,FORK_HEIGHT, difficulty_guess);  }
		if (DA == "DIGISHIELD_" ) { next_D = DIGISHIELD_(TS,CD,T,N, height, FORK_HEIGHT, difficulty_guess);  }  
		if (DA == "DIGISHIELD_improved_" ) { next_D = DIGISHIELD_improved_(TS,CD,T,N,height,FORK_HEIGHT, difficulty_guess);  } 
		if (DA == "SMA_" ) {	next_D = SMA_(TS,CD,T,N, height, FORK_HEIGHT, difficulty_guess);  }
		if (DA == "SMS_" ) {	next_D = SMS_(TS,CD,T,N, height, FORK_HEIGHT, difficulty_guess);  }
		if (DA == "EMA_" ) {	next_D = EMA_(TS,CD,T,N, height, FORK_HEIGHT, difficulty_guess);  }
		if (DA == "EMA3_" ) {	next_D = EMA3_(TS,CD,T,N, height, FORK_HEIGHT, difficulty_guess);  }  
		if (DA == "ETH_" ) {	next_D = ETH_(TS,CD,T,N, height, FORK_HEIGHT, difficulty_guess);  }
		if (DA == "LWMA_ASERT_" ) {	next_D = LWMA_ASERT_(TS,CD,T,N, height, FORK_HEIGHT, difficulty_guess, R);  }
		if (DA == "ASERT_" || DA == "ASERT_RTT_" ) {	next_D = ASERT_(TS,CD,T,N, height, FORK_HEIGHT, difficulty_guess, R);  } 
		if (DA == "ASERT_SMA_" ) {	next_D = ASERT_SMA_(TS,CD,T,N, height, FORK_HEIGHT, difficulty_guess, R);  }
		if (DA == "DGW_" ) {	next_D = DGW_(TS,CD,T,N, height, FORK_HEIGHT, difficulty_guess);  }
		//  **** Begin TSA section  ****
		if (DA == "ASERT_RTT_") {  }
		if (DA == "TSA_" ) { 
				// TSA is set up for use with LWMA1 only.  
				// It can't simulate ST with CN_delay because it depends on a per-block ST & D connection.
				USE_CN_DELAY = 0;
				// CONSTANT_HR=1; //  This was for a different type of miner "attack" used in simulated_ST().
				next_D = EMA_(TS,CD,T,N, height, FORK_HEIGHT, difficulty_guess);
			float mR = R;
			float mT = T;
			simulated_ST = static_cast<u>(0.5+mT*mR*log(1+pow(2.7183,1/mR)*NEG_LOG_RAND[i]*
							static_cast<float>(next_D*DX/mT/mR/HR)));
				current_ST = simulated_ST; 
				 u template_timestamp = current_ST + TS.back();
			TSA_D =  TSA_(TS,CD,T,N, height, FORK_HEIGHT, difficulty_guess, template_timestamp, R);  
		 } 
		// <-- false indention
		//  **** End TSA section  ****

		// CD gets 1 block ahead of TS
		CD.push_back(CD.back() + next_D);
		if (CD.size() > N+1 ) { CD.erase(CD.begin()); }

		// Simulate solvetime for this next_D
 
		if (DA != "TSA_" && DA != "ASERT_RTT_" )  { 
			simulated_ST = static_cast<u>(NEG_LOG_RAND[i]*(CD.back()-CD[CD.size()-2])*DX/HR);
		}
		if (USE_CN_DELAY) {  current_ST = previous_ST; previous_ST = simulated_ST; }
		else { current_ST = simulated_ST; }  // TSA always
		// TS catches up with CD
		if (DA != "ASERT_RTT_" ) {	TS.push_back(TS.back() + current_ST); 
			if (TS.size() > N+1 ) { TS.erase(TS.begin()); }
		}
		// Ds[i]  = CD.back() - CD[CD.size()-2];
		Ds[i] = next_D; 
		if (DA == "TSA_" ) { Dtsa[i] = TSA_D; } // Dtsa[i] = CD.back() - CD[CD.size()-2];

		HRs[i] = HR;	
		STs[i] = current_ST;
		if (i>2*N) {
			avgST += current_ST;
			avgD  += Ds[i];
			avgHR += HR;
			avgDtsa += Dtsa[i];
		}
		
		if (PRINT_BLOCKS_TO_COMMAND_LINE) {  cout << i << "\t" << STs[i] << "\t" << Ds[i] << endl;	}
		// if (Ds[i] < 100 ) { cout << " D went < 100 at iteration " << i << " " << Ds[i] << endl; }
	}
	avgST /= (BLOCKS-2*N); avgHR /= (BLOCKS-2*N); avgD /= (BLOCKS-2*N); avgDtsa /= (BLOCKS-2*N);
	cout << DA << " " << N << " avg ST: " << avgST << " avg Diff: " << avgD << " ";
	string temp = "blocks_" + DA + ".txt";	ofstream blocks_file(temp);

	int64_t j=0;
	vector<int64_t>histotimes(6*T);
	vector<float>nD(BLOCKS), nDtsa(BLOCKS), nST(BLOCKS), nHR(BLOCKS), nST11(BLOCKS), nAttack(BLOCKS);
	float SD(0),SD_ST(0);
	float delays=0, stolen=0, dedicated_reward=0, attackers_reward=0, dedicated_time=0, attackers_time=0;
	int64_t attack_blocks=0; 

	for (i=2*N+1;i<BLOCKS;i++) {	
		nD[i] = round(Ds[i]*100/avgD)/100;
		nDtsa[i] = round(Dtsa[i]*100/avgDtsa)/100;
		nHR[i] = round(HRs[i]*100/baseline_HR)/100;
		nST[i] = round(STs[i]*100/T)/100; 
		// if (nHR[i] == 1) { 	// I decided this is not correct
		dedicated_time += nST[i];
		DA == "TSA_" ? dedicated_reward += nST[i]/nDtsa[i] : dedicated_reward += nST[i]/nD[i];
		// }
		// cout << "==== " << attackers_reward << endl;
		// total_time += nST[i];
		if (nHR[i] > 1.001) { 	attackers_time += nST[i]; attack_blocks++;
			DA == "TSA_" ? attackers_reward += nST[i]/nDtsa[i] : attackers_reward += nST[i]/nD[i]; 
		}
		if ( nST[i] > 4	) { delays += nST[i] - 4; } // This is the delay metric. 
		if ( STs[i] < 6*T ) { histotimes[STs[i]]++;} //	histogram
		if (DA == "TSA_" ) { Dtsa[i] =round(Dtsa[i]*100/avgD)/100; }
		// if (nST[i] > 8 ) { cout << " ST > 8xT " << nST[i] << endl; }
 
		if (i >= 5 && i < BLOCKS-5) { 
			for (int j = i-5; j <= i+5; j++) { nST11[i] += STs[j]; };
			nAttack[i] = (11*T)/(nST11[i]+1);
			nAttack[i] = round(nAttack[i]*100)/100;
			nST11[i]	= round((nST11[i]*100)/(T*11))/100; 
			if ( nST11[i] > 1.9	) { delays += (nST[i] - 1); }
			//	if ( nST11[i] < 0.43 && nD[i] < 1) { stolen += 1 - nD[i]; }
			// if ( nST11[i] < 0.43) { stolen += 1 - nST11[i]; }
			//	if ( i > 5+4 && nST11[i] < 0.43 && nST11[i-1] < 0.43 && nST11[i-2] < 0.43 && nST11[i-3] >= 0.43 ) { stolen += (1 - nD[i])*5; } // accounts for underestimate in method
			//	nST11[i] = accumulate(nST[i-5],nST[+5])/11; nAttack[i]=1/nST11[i]; 
		}  
		if (ENABLE_FILE_WRITES) { blocks_file << i+FORK_HEIGHT << "\t" << STs[i] << "\t" << Ds[i] << endl;} 
		SD += (Ds[i]-avgD)*(Ds[i]-avgD)/BLOCKS/avgD/avgD;
		SD_ST += (STs[i]-avgST)*(STs[i]-avgST)/BLOCKS/avgST/avgST;
	}

	stolen =round(10000*((attackers_reward/attackers_time)/(dedicated_reward/dedicated_time)-1))/100;
	delays = round(10000*delays/(BLOCKS-2*N))/100;
	float stolen_blocks = stolen*attack_blocks/100;

	cout << delays << "% delays. " << attack_blocks << 
	" attack_blocks. " << stolen << "% cheaper (" << stolen_blocks << " 'free' blocks) for on-off mining. " << endl;

	if (ENABLE_FILE_WRITES) { blocks_file.close(); }

	ofstream histo_file("histogram" + DA + to_string(IDENTIFIER) + ".txt");
	for (int i=0;i<6*T;i++) { histo_file << i << "\t" << histotimes[i] << endl; }
	histo_file.close();

	if (ENABLE_FILE_WRITES) {
		temp = "plot_" + DA + to_string(IDENTIFIER) + ".txt";	ofstream plot_file(temp);
		for (i=2*N+1;i<BLOCKS;i++) {	
			plot_file << i+FORK_HEIGHT << "\t" << nD[i] << "\t" << nST[i] << "\t" << nHR[i] << "\t" << nST11[i] << "\t" << nAttack[i] << "\t" << Dtsa[i] << endl;
		}
		plot_file.close();
		int spacing = BLOCKS/60;
		int end = FORK_HEIGHT+BLOCKS;
		temp = "gnuplot -c test_DAs_gnuplot.txt " + to_string(FORK_HEIGHT) + " " + to_string(end) + " " + 
			to_string(spacing) + " " + DA + to_string(IDENTIFIER) + " " + to_string(11) ;
		system((temp).c_str() );
		// if ( DA == "DIGISHIELD_" ) { N=N-6; }
		html_file << "<B>" << DA << "</B> Target ST/avgST= " << T << "/" << avgST << " N= " << N;  
		if (attack_size != 100) { html_file << " attack_size: " << attack_size << " start/start: " 
		 << attack_start << "/" << attack_stop;}
		html_file << " StdDev Diffs: " << round(1000*sqrt(SD))/1000 << " StdDev STs: " << 
		round(100*sqrt(SD_ST))/100 << " delays: " << delays << "% stolen: " << stolen << 
		"%  TWAavgD: " << round(1000*dedicated_time/dedicated_reward*avgD)/1000 << "  M=" << R << "\n<br><img src=gif_" << 
		DA << to_string(IDENTIFIER) << ".gif><br>" << endl;
	}
} 

int main() 
{
u N; string DA; srand(time(0)); // seed for fRand();
u M = 0; // extra parameter for some algos

// These global constants are not typically changed for a given set of simulations.

BASELINE_D = 1e4; // The average D just before the fork. 
if (BASELINE_D < 10) { cout << "BASELINE_D needs to be > 10 because of they way HR is used" << endl; return 0; }
u difficulty_guess = BASELINE_D; 
FORK_HEIGHT = 0; // if 0, pretend it is genesis.
START_TIMESTAMP = 1540000000; 
START_CD = 1E9; // Beginning cumulative_difficulty. 0 is fine. 
DX = 1;  // DX=1 for CN coins. DX = 2^32 for bitcoin and 2^13 for Zcash
ENABLE_FILE_WRITES = 1;


PRINT_BLOCKS_TO_COMMAND_LINE = 0;

u T = 600;

u baseline_HR = (BASELINE_D*DX)/T; // baseline_HR = (BASELINE_D*DX)/T; to match HR to baseline D
if (baseline_HR < 2) { cout << "baseline_D/T ratio is too small to simulate with an integer HR." << endl; exit(0); }
USE_CN_DELAY = 0; // 1 = yes, 0=no. CN coins have a delay on timestamps


BLOCKS = 20000; //################################## BLOCKS to simulate

// Get -ln(x) values for all algos #####
for (int i=0; i <= BLOCKS-1; i++) { NEG_LOG_RAND.push_back(log( 1/fRand(0.000001,0.999999))); }
// Do not plot if run is > 30,000
if (BLOCKS > 30000) { ENABLE_FILE_WRITES = 0; PRINT_BLOCKS_TO_COMMAND_LINE = 0;}
if (ENABLE_FILE_WRITES) { html_file << "<HTML><head><title>Difficulty Plots</title></head><body><br>" << endl; }

//  ************** Set attack size ***************
u attack_start = 130; // 90 = 90% of baseline D.  
u attack_stop = 135; // 120 = 120% of baseline D.
u attack_size = 800; // HR multiple in percent of baseline HR. SET TO 100 for NO ATTACKS

// ********* Select Algos and Run **********
 
T=600;
/*
DA = "LWMA1_";
N = 160; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0); 

DA = "ETH_"; 
N = 110; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0); 
*/

/* for ( attack_start = 80; attack_start <= 130; attack_start+=5) {
	for ( attack_stop = 80; attack_stop <= 130; attack_stop+=5) {
	cout << " start: " << attack_start << " stop: " << attack_stop << " ";
*/
DA = "SMA_";
N = 144; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0); 

DA = "DIGISHIELD_";  // INCLUDES THE MTP 6 BLOCK DELAY !!!
N = 17; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0);

DA = "LWMA1_";
N = 60; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0); 

DA = "EMA_";
N = 100; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0); 


/*
DA = "DIGISHIELD_improved_"; 
N = 15; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0);

DA = "ASERT_";
N = 10; IDENTIFIER++; M=32;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, M); 
*/

// }} for checing all attack scenarios

html_file.close();
exit(0); 
} ;

/*
DA = "ASERT_"; 
N = 10; IDENTIFIER++;	 M=80;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, M);

DA = "ASERT_SMA_";
N = 10; IDENTIFIER++;  M=80;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, M);

DA = "LWMA1_";
N = 144; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0); 

DA = "KGW_";
N = 144; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0); 

DA = "ETH_"; 
N = 100; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0); 

DA = "DIGISHIELD_improved_";
N = 26; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0);

// ================

DA="TSA";

// Unlike others, attack_start is relative to avg of N difficulties and is based on 
// TSA_D for a given timestamp
R = 1;  // only 1, 2, and 4 supported in simulation. This is only needed for TSA.
N = 600; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size,R);

R = 2;  // only 2 and 4 supported in simulation. This is only needed for TSA.
CONSTANT_HR =0;
N = 600; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size,R);

R = 4;  // only 2 and 4 supported in simulation. This is only needed for TSA.
CONSTANT_HR =0;
N = 600; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size,R);

*/
