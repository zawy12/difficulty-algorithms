// Timespan Limit Attack Demonstration
// Copyright (c) Zawy 2019
// MIT License
/*
Shows how a >50% selfish mining attack can get unlimited number of blocks in about 3x the 
difficulty window by retarding the MTP and using timespan limits against themselves.
It currently only tests symmetrical limits like BTC's 4x and 1/4 and DASH's 3x, and 1/3.
A future update may include fixed-window algos like BTC and LTC which are easier to attack, 
and asymmetrical fractional limits like Digishields that also make it a lot easier.
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
d run_DA (u choose_DA, d targets[], u S[], u N, u T, u L, u h) {
	if (choose_DA == 1) { BCH(targets, S, N, T, L, h); }
	if (choose_DA == 2) { SMA(targets, S, N, T, L, h); }
	if (choose_DA == 3) { DGW(targets, S, N, T, L, h); }
	if (choose_DA == 4) { Digishield(targets, S, N, T, L, h); }
	if (choose_DA == 5) { LWMA(targets, S, N, T, L, h); }
/*	if (choose_DA == 6) { Boris(targets, S, N, T, L, h); }
	if (choose_DA == 7) { BTC(targets, S, N, T, L, h); }
	if (choose_DA == 8) { LTC(targets, S, N, T, L, h); }
	if (choose_DA == 9) { ETH(targets, S, N, T, L, h); }   */
}
int main () {
srand(time(0));

u test_DA = 0; // set to 1 to test DA code without the attack

u N = 144; // difficulty averaging window, use > MTP
u L = 2; // timespan limit. BTC=4, BCH=2, DASH / DGW=3. Symmetrical assumed.
u T = 600; // block time
u MTP = 11; // most coins use MTP=11 (median of the past 11 timestamps)

// The following is adjusted to increase the # of blocks gained or decrease the time required.
// Select a target number of blocks (via blocks = 10*N below) then keep adjusting the following
// until the max timestamp you see in left column equals real time in far right colmun.
// That's when the chain is submitted.

u adjust = 103;  // this is critical. A change of 1% can double the blocks. Typical range: 50 to 150.

/*  The DAs I use do not use MTP as most recent timestamp or checkpoints.
All Digishields have the MTP delay, so they are not vulnerable to this attack.

1 = BCH
2 = SMA
3 = Digishield // not coded for yet 
4 = DGW
5 = LWMA // not coded for yet
BTC, LTC, ETH // not coded for yet  */

u choose_DA = 1; // Choose a number from the above list.

if (choose_DA == 1) { T=600; N=144; L=2; }  // BCH adjust = 103 got 70k blocks in 3.5 days; 
if (choose_DA == 4) { N=24; L=3;  }   // DGW

u blocks = 3*N; // How many blocks you want to get. (try = 10*N at first)
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

cout << "Initialize N + MTP blocks:\nheight,\ttimestamps,\tdifficulty,\ttarget,\tsolvetime\n"; 

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
	cout << h << ",\t" << S[h] << ",\t" << difficulty << ",\t" << targets[h] 
	<< ",\t" << S[h]-S[h-1] << endl; 
   // cout << h << " " << S[h] << " " <<  round(1000*difficulty/avg_initial_diff)/1000 << " " << solvetime << endl;
}
u MTP_array[MTP];
u MTP_stamp = 0;
u MTP_next = 1; 
u MTP_previous = 0;
u j = 0;

if (test_DA == 1) { cout << "begin actual difficulty calculations\n"; }
else { 
	cout << "Set time and timestamp = 0 on block prior to this attack. \nheight\t timestamp\t apparent solvetime\t MTP\t normalized Difficulty\t actual solvetime\t real time\t minutes into attack\n";
}
u M = L*N*T; // A useful constant
// The following is the attacker's first timestamp. It is forward in time, but 
// it becomes the "held-back" MTP that is the key to the attack's success
u Q = S[N+MTP-N] + M*adjust/100; 
for (u h = N+MTP; h < blocks+N+MTP; h++ ) {
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

		// First N blocks have to be done a certain way to maximize future gains.
		if ( j==0 ) { S[h]= Q; } // begin attack 
		else if ( j <= N ) {
			// Calculate MTP of next block
			MTP_array[0] = min(Q + M, M+S[h-N]);
			for (u i = 1; i<MTP; i++) { MTP_array[i] = S[h-i]; }
			MTP_next = median(MTP_array, MTP); 

			// This is the key code for 1st N blocks.
			if ( MTP_next <= Q && MTP_next >= MTP_previous &&
				Q == S[h-1]+1 ) { S[h] =  M + S[h-N]; } // jumps to a forward time if MTP is safely delayed.
			else { S[h] = Q; }  // holds back the MTP
		}
		// After 1st N blocks, do the sustained attack pattern.
		// It's possible to use the following alone to replace the above, but 
		// attack takes 2x longer. 
		else  {
			MTP_array[0] = Q + M;
			for (u i = 1; i<MTP; i++) { MTP_array[i] = S[h-i]; }
			MTP_next = median(MTP_array, MTP); 
			
			// This is the key code for the sustained attack.
			if ( MTP_next <= Q && MTP_next >= MTP_previous && 
					Q == S[h-N]+N )  {  S[h] = M + Q;  }  // jumps to our forward "submit" time if MTP is safely delayed.
			else { S[h] = Q; }  // holds back the MTP
		}
		j++;
		Q += 1; // Because protocol requires timestamps >= MTP + 1 second 
	}
	if (test_DA == 0) {
		for (u i = 0; i<MTP; i++) { MTP_array[i] = S[h-i];  }
		MTP_next = median(MTP_array, MTP);
	
		difficulty = powLimit/targets[h];
		if (j == 1) { MTP_previous = S[N+MTP-1]-6*T+T; }  // an estimate to not make output confusing.
	
		cout << h-N-MTP << "\t" << S[h]-S[N+MTP-1] << "\t" << S[h]-S[h-1] << "\t" << MTP_previous-S[N+MTP-1]
		<< "\t" <<  round(1000*difficulty/avg_initial_diff)/1000 << "\t" 
		<< round(solvetime) << "\t" << real_time[h]-S[N+MTP-1]<< "\t"<< 
		round((real_time[h] - real_time[MTP+N-1])/60) << endl;

		/*  Alternate Debugging view of output
		cout << h-N-MTP << "\t" << S[h] << "\t" << S[h]-S[h-1] << "\t" << MTP_previous  
		<< "\t" <<  round(1000*difficulty/avg_initial_diff)/1000 << "\t" 
		<< round(100*solvetime)/100 << "\t" << real_time[h] << "\t"<< 
		round((real_time[h] - real_time[MTP+N])/60) << endl;
		*/

		if (MTP_next < MTP_previous) { cout << "MTP_next (" << MTP_next << ") is smaller than " 
		<< MTP_previous << ". Adjust setting was too small.\n"; exit(0);}
		MTP_previous = MTP_next;
	}
}
if (test_DA ) { cout << "Avg solvetime: " << (S[blocks+N+MTP-1]-S[N+MTP])/(blocks-2) << endl; }
else { cout << "height\t timestamp\t apparent solvetime\t MTP\t normalized Difficulty\t actual solvetime\t real time\t minutes into attack\n"; }
return(0);
}
