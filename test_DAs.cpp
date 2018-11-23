#include <iostream> // needed for cout << endl.
#include <vector>
#include <fstream> 
#include <bits/stdc++.h> // for sorting vectors
#include <string> 
#include <math.h>  // needed for log()
#include <cassert>  // wownero said this was needed
using namespace std; // removes need for std:: but std:: should be retained in DAs.

typedef uint64_t difficulty_type;
typedef uint64_t u;

string temp = "test_DAs.html";
ofstream html_file(temp);

/*
This simulates running cryptonote-type difficulty algorithms.
See main() to select algorithm(s) and settings.  
It can simulate on-off mining. 
It outputs timestamps and difficulties to screen and file.
Compile and running with 
g++ -std=c++11 test_DAs.cpp -o test_DAs && ./test_DAs
*/

// Allow some global constants for all simulations.
// These are set in main() so that there's only 1 place to change variables

u BLOCKS, FORK_HEIGHT, START_TIMESTAMP, START_CD, BASELINE_D, USE_CN_DELAY, DX, ENABLE_FILE_WRITES; 
u IDENTIFIER(0);


float fRand(float fMin, float fMax) {   
		float f = (float)rand() / RAND_MAX;
    return fMin + f * (fMax - fMin);
}
// The following is not currently used.
uint64_t solvetime_without_exploits (std::vector<uint64_t>timestamps, uint64_t T) {
	uint64_t previous_timestamp(0), this_timestamp(0), ST, i;
	previous_timestamp = timestamps.front()-T;
	for ( i = 0; i < timestamps.size(); i++) {
  	 if ( timestamps[i] > previous_timestamp ) { this_timestamp = timestamps[i]; } 
  	 else { this_timestamp = previous_timestamp; }
     ST = std::max(1+T/50,this_timestamp - previous_timestamp);
  	 previous_timestamp = this_timestamp; 
	}
	return ST;
}
// Simple Moving average
// This will have a slightly slow avg ST for small N because it is terms of D instead of hash target.
// Harmonic D could be used, but it's not hardly possible with integer math. To work in terms of 
// target, it would probably require 2^256 math which is way beyond uint64_t.
difficulty_type SMA_(std::vector<uint64_t> timestamps, 
   std::vector<uint64_t> cumulative_difficulties, uint64_t T, uint64_t N, uint64_t height,
					uint64_t FORK_HEIGHT,uint64_t difficulty_guess) {
   // Genesis should be the only time sizes are < N+1.
   assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );

   // Hard code D if there are not at least N+1 BLOCKS after fork (or genesis)
   if (height < FORK_HEIGHT + N) { return difficulty_guess; }
   assert(timestamps.size() == N+1); 
	uint64_t ST = std::max(N,timestamps[N] - timestamps[0]);
	return uint64_t (cumulative_difficulties[N]-cumulative_difficulties[0])*T / ST;
}
// Dark Gravity Wave
// DGW_ uses an insanely complicated loop that is a simple moving average with double
// weight given to the most recent difficulty, which has virtually no effect.  So this 
// is just a SMA_ with the 1/3 and 3x limits in DGW_.
difficulty_type DGW_(std::vector<uint64_t> timestamps, 
   std::vector<uint64_t> cumulative_difficulties, uint64_t T, uint64_t N, uint64_t height,
					uint64_t FORK_HEIGHT,uint64_t  difficulty_guess) {
   // Genesis should be the only time sizes are < N+1.
   assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );

   // Hard code D if there are not at least N+1 BLOCKS after fork (or genesis)
   if (height < FORK_HEIGHT + N) { return difficulty_guess; }
   assert(timestamps.size() == N+1); 

	uint64_t ST = std::max(N,timestamps[N] - timestamps[0]);
  ST = std::max((N*T)/3,std::min((N*T)*3,ST));
	return uint64_t (cumulative_difficulties[N]-cumulative_difficulties[0])*T / ST;
}

// Digishield
// Digishield uses the median of past 11 timestamps as the beginning and end of the window.
// This only simulates that, assuming the timestamps are always in the correct order.
// Also, this is in terms of difficulty instead of target, so a "99/100" correction factor was used.
difficulty_type DIGISHIELD(std::vector<uint64_t> timestamps, 
   std::vector<uint64_t> cumulative_difficulties, uint64_t T, uint64_t N, uint64_t height,
					uint64_t FORK_HEIGHT,uint64_t  difficulty_guess) {

   // Genesis should be the only time sizes are < N+1.
   assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );

   // Hard code D if there are not at least N+1 BLOCKS after fork (or genesis)
   if (height < FORK_HEIGHT + N) { return difficulty_guess; }
   assert(timestamps.size() == N+1); 

	uint64_t ST = std::max(N,timestamps[N-6] - timestamps[0]);
  ST = std::max(((N-6)*T)*84/100,std::min((N-6)*T*132/100,ST));
	return uint64_t (cumulative_difficulties[N]-cumulative_difficulties[6])*T*400/(300*(N-6)*T+99*ST);
}

// Digishield without MTP=11 delay and no limits
difficulty_type DIGISHIELD_impoved_(std::vector<uint64_t> timestamps, 
   std::vector<uint64_t> cumulative_difficulties, uint64_t T, uint64_t N, uint64_t height,
					uint64_t FORK_HEIGHT,uint64_t  difficulty_guess) {
   // Genesis should be the only time sizes are < N+1.
   assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );

   // Hard code D if there are not at least N+1 BLOCKS after fork (or genesis)
   if (height < FORK_HEIGHT + N) { return difficulty_guess; }
   assert(timestamps.size() == N+1); 

	uint64_t ST = std::max(N,timestamps[N] - timestamps[0]);
	return uint64_t (cumulative_difficulties[N]-cumulative_difficulties[0])*T*400/(300*N*T+100*ST);
}

//  EMA_ difficulty algorithm
difficulty_type EMA_(std::vector<uint64_t> timestamps, 
   std::vector<uint64_t> cumulative_difficulties, uint64_t T, uint64_t N, uint64_t height,
					uint64_t FORK_HEIGHT,uint64_t  difficulty_guess) {
  // The last 3 input variables above are not used. They're their for consistency with the other DAs.
  assert(N>2); 
	assert(difficulty_guess > 9*N); // D must be > 9xN
	
	// Safely prevent out of sequence timestamps.
	uint64_t previous_timestamp(0), this_timestamp(0), ST, i;
	previous_timestamp = timestamps.front()-T;
	for ( i = 0; i < timestamps.size(); i++) {
  	 if ( timestamps[i] > previous_timestamp ) { this_timestamp = timestamps[i]; } 
  	 else { this_timestamp = previous_timestamp; }
     ST = std::max(1+T/100,std::min(10*T,this_timestamp - previous_timestamp));
  	 previous_timestamp = this_timestamp; 
	}
 
	/* Calculate e^(t/T)*1000 using integer math w/o pow() to
	ensure cross-platform/compiler consistency.
	This approximation comes from
	e^x = e^(floor(x)) * ef  
	where ef = 1 + f + f*f/2 + f*f*f/6 = 1+f(1+f(1+f/3)/2) 
	where f=1 for mod(x)=0 else f=mod(x) where x = t/T
*/
	uint64_t s = 100000;
	uint64_t exk = s; // exk = e^x * 1000
	for ( int i = 1; i <= ST/T/N ; i++ ) { 	exk = (exk*271828)/s; 	} // 27183 must be same # digitits as s
	uint64_t tm = ST % (T*N); 
	exk = (exk*(s+(tm*(s+(tm*(s+(tm*(s+(tm*s)/(4*T*N)))/(3*T*N)))/(2*T*N)))/(T*N)))/s;
	uint64_t prev_D = cumulative_difficulties.back() - cumulative_difficulties[cumulative_difficulties.size()-2];
	uint64_t next_D = std::max(static_cast<uint64_t>(10),(s*prev_D*ST)/(s*T+(ST-T)*exk)); 
	// For non-small N, this works as well as all the exk mess.
  // uint64_t next_D = std::max(static_cast<uint64_t>(1),(prev_D*N*T)/(T*N+ST-T)); // TH method who's denominator can't go negative.
	return next_D;  	
}

// LWMA difficulty algorithm 
// Copyright (c) 2017-2018 Zawy, MIT License
// https://github.com/zawy12/difficulty-algorithms/issues/3
// See commented version for explanations & required config file changes. Fix FTL and MTP!
// REMOVE COMMENTS BELOW THIS LINE. 
// The options are recommended. They're called options to show the core is a simple LWMA.
// Bitcoin clones must lower their FTL. 
// Cryptonote et al coins must make the following changes:
// #define BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW    11
// #define CRYPTONOTE_BLOCK_FUTURE_TIME_LIMIT        3 * DIFFICULTY_TARGET 
// #define DIFFICULTY_WINDOW         90 //  N=60, 90, and 120 for T=600, 120, 60.
// Warning Bytecoin/Karbo clones may not have the following, so check TS & CD vectors size=N+1
// #define DIFFICULTY_BLOCKS_COUNT       DIFFICULTY_WINDOW+1
// The BLOCKS_COUNT is to make timestamps & cumulative_difficulty vectors size N+1
// Do not sort timestamps.  
// CN coins (Monero < 12.3) must deploy the Jagerman MTP Patch. See:
// https://github.com/loki-project/loki/pull/26   or
// https://github.com/graft-project/GraftNetwork/pull/118/files

difficulty_type LWMA1_(std::vector<uint64_t> timestamps, 
   std::vector<uint64_t> cumulative_difficulties, uint64_t T, uint64_t N, uint64_t height,  
					uint64_t FORK_HEIGHT,uint64_t  difficulty_guess) {
    
   // This old way was not very proper
   // uint64_t  T = DIFFICULTY_TARGET_V2;
   // uint64_t  N = DIFFICULTY_WINDOW_V2; // N=60, 90, and 120 for T=600, 120, 60.
   
   // Genesis should be the only time sizes are < N+1.
   assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );

   // Hard code D if there are not at least N+1 BLOCKS after fork (or genesis)
   if (height < FORK_HEIGHT + N) { return difficulty_guess; }
   assert(timestamps.size() == N+1); 
   

   uint64_t  L(0), next_D, i, this_timestamp(0), previous_timestamp(0), avg_D;
   // If hashrate/difficulty ratio after a fork is < 1/3 prior ratio, hardcode D for N+1 BLOCKS after fork. 
   // This will also cover up a very common type of backwards-incompatible fork.
   // difficulty_guess = 100000; //  Dev may change.  Guess low than anything expected.
   // if ( height <= UPGRADE_HEIGHT + 1 + N && height >= UPGRADE_HEIGHT ) { return difficulty_guess;  }
 
	previous_timestamp = timestamps[0]-T;
	for ( i = 1; i <= N; i++) {        
		// Safely prevent out-of-sequence timestamps
		if ( timestamps[i]  > previous_timestamp ) {   this_timestamp = timestamps[i];  } 
		else {  this_timestamp = previous_timestamp;   }
		L +=  i*std::min(6*T ,this_timestamp - previous_timestamp);
		previous_timestamp = this_timestamp; 
	}
	if (L < N*N*T/20 ) { L =  N*N*T/20; }
	avg_D = ( cumulative_difficulties[N] - cumulative_difficulties[0] )/ N;
   
	// Prevent round off error for small D and overflow for large D.
	if (avg_D > 2000000*N*N*T) { 
		next_D = (avg_D/(200*L))*(N*(N+1)*T*99);   
	}   
	else {    
		next_D = (avg_D*N*(N+1)*T*99)/(200*L);    
	}	

	// Optional. Make all insignificant digits zero for easy reading.
	i = 1000000000;
	while (i > 1) { 
		if ( next_D > i*100 ) { next_D = ((next_D+i/2)/i)*i; break; }
		else { i /= 10; }
	}
	// Make least 2 digits = size of hash rate change last 11 BLOCKS if it's statistically significant.
	// D=2540035 => hash rate 3.5x higher than D expected. Blocks coming 3.5x too fast.
	if ( next_D > 10000 ) { 
		uint64_t est_HR = (10*(11*T+(timestamps[N]-timestamps[N-11])/2)) / 
                                   (timestamps[N]-timestamps[N-11]+1);
		if (  est_HR > 5 && est_HR < 25 )  {  est_HR=0;   }
		est_HR = std::min(static_cast<uint64_t>(99), est_HR);
		next_D = ((next_D+50)/100)*100 + est_HR;  
	}
	return  next_D;
}

// LWMA-4 difficulty algorithm 
// Copyright (c) 2017-2018 Zawy, MIT License
// https://github.com/zawy12/difficulty-algorithms/issues/3
// See commented version for explanations & required config file changes. Fix FTL and MTP!

difficulty_type LWMA4_(std::vector<uint64_t> timestamps, 
   std::vector<difficulty_type> cumulative_difficulties, uint64_t T, uint64_t N, uint64_t height,  
					uint64_t FORK_HEIGHT,uint64_t  difficulty_guess) {
    
   // This old way was not very proper
   // uint64_t  T = DIFFICULTY_TARGET_V2;
   // uint64_t  N = DIFFICULTY_WINDOW_V2; // N=60, 90, and 120 for T=600, 120, 60.

    
   // Genesis should be the only time sizes are < N+1.
   assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );

   // Hard code D if there are not at least N+1 BLOCKS after fork (or genesis)
   if (height < FORK_HEIGHT + N) { return difficulty_guess; }
   assert(timestamps.size() == N+1); 

   uint64_t  L(0), ST(0), next_D, prev_D, avg_D, i;
        
   
   // If hashrate/difficulty ratio after a fork is < 1/3 prior ratio, hardcode D for N+1 BLOCKS after fork. 
   // This will also cover up a very common type of backwards-incompatible fork.
   // difficulty_guess = 100000; //  Dev may change.  Guess low than anything expected.
   // if ( height <= UPGRADE_HEIGHT + 1 + N ) { return difficulty_guess;  }
 
   // Safely convert out-of-sequence timestamps into > 0 solvetimes.
   std::vector<uint64_t>TS(N+1);
   TS[0] = timestamps[0];
   for ( i = 1; i <= N; i++) {        
      if ( timestamps[i]  > TS[i-1]  ) {   TS[i] = timestamps[i];  } 
      else {  TS[i] = TS[i-1];   }
   }

   for ( i = 1; i <= N; i++) {  
      // Temper long solvetime drops if they were preceded by 3 or 6 fast solves.
      if ( i > 4 && TS[i]-TS[i-1] > 5*T  && TS[i-1] - TS[i-4] < (14*T)/10 ) {   ST = 2*T; }
      else if ( i > 7 && TS[i]-TS[i-1] > 5*T  && TS[i-1] - TS[i-7] < 4*T ) {   ST = 2*T; }
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
   else {    next_D = (avg_D*N*(N+1)*T*97)/(200*L);    }

   prev_D =  cumulative_difficulties[N] - cumulative_difficulties[N-1] ; 

   // Apply 10% jump rule.
   if (  ( TS[N] - TS[N-1] < (2*T)/10 ) || 
         ( TS[N] - TS[N-2] < (5*T)/10 ) ||  
         ( TS[N] - TS[N-3] < (8*T)/10 )    )
   {  
       next_D = std::max( next_D, std::min( (prev_D*110)/100, (105*avg_D)/100 ) ); 
   }
   // Make all insignificant digits zero for easy reading.
   i = 1000000000;
   while (i > 1) { 
     if ( next_D > i*100 ) { next_D = ((next_D+i/2)/i)*i; break; }
     else { i /= 10; }
   }
   // Make least 3 digits equal avg of past 10 solvetimes.
   if ( next_D > 100000 ) { 
    next_D = ((next_D+500)/1000)*1000 + std::min(static_cast<uint64_t>(999), (TS[N]-TS[N-10])/10); 
   }
   return  next_D;
}

int run_simulation(string DA, u T, u N,u difficulty_guess,u baseline_HR,u attack_start,u attack_stop,u attack_size) {

if ( DA == "DIGISHIELD_" ) { N=N+6; } // For simulated MTP = 11 delay.

	// In theory, this just uses ST = -ln(rand())* T * D/HR, but it's a mess, half-way because of CN_delay.
  // I should have made two different methods.  One with CN delay and one without.
  // Even without that it would have been a mess because of genesis or fork option.  Needed 4 methods.

	u next_D, simulated_ST(0), i, HR(0), attack_on(0);
	float avgST(0), avgHR(0), avgD(0);
	u previous_ST, current_ST;
	vector<u>STs(BLOCKS);
  vector<u>Ds(BLOCKS);
	vector<u>HRs(BLOCKS);

	// Initially assume it's genesis
	vector<u>TS(1);
	TS[0] = START_TIMESTAMP;

	vector<u>CD(1);
	CD[0] = START_CD; 

	// If it's a fork initialize previous N BLOCKS
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
		cout << "You can't fork if genesis was < N+1 BLOCKS in the past." << endl; 
	}
// *****  Run simulation  ********
previous_ST = T; // initialize in case we are using CN delay.
HR = baseline_HR;
u height = FORK_HEIGHT; // This is kind of dummy work to send DA's what they need.
for (i=0; i <= BLOCKS-1; i++) {
	// attack_size = 0 means there's no hash attack, but just constant HR.
	if (next_D > (attack_stop*BASELINE_D)/100  ) { 
		attack_on = 0;
		HR = baseline_HR;
	}
	else if (next_D < (attack_start*BASELINE_D)/100 ) {
		attack_on = 1;
		HR = (baseline_HR*attack_size)/(100);
	}
  // if (HR == 0) { HR=1; cout << "HR was zero, so it was changed to 1 to prevent 1/0." << endl; }
  // Run DA
  
	height = FORK_HEIGHT+i;
  if ( i <= BLOCKS-1 ) { 

		if (DA == "LWMA1_" ) { next_D = LWMA1_(TS,CD,T,N, height, FORK_HEIGHT, difficulty_guess);  }
		if (DA == "LWMA4_" ) { next_D = LWMA4_(TS,CD,T,N,height,FORK_HEIGHT, difficulty_guess);  }
		if (DA == "DIGISHIELD_" ) { next_D = DIGISHIELD(TS,CD,T,N, height, FORK_HEIGHT, difficulty_guess);  }  
		if (DA == "DIGISHIELD_impoved_" ) { next_D = DIGISHIELD_impoved_(TS,CD,T,N,height,FORK_HEIGHT, difficulty_guess);  } 
		if (DA == "SMA_" ) { next_D = SMA_(TS,CD,T,N, height, FORK_HEIGHT, difficulty_guess);  }
		if (DA == "EMA_" ) { next_D = EMA_(TS,CD,T,N, height, FORK_HEIGHT, difficulty_guess);  }
		if (DA == "DGW_" ) { next_D = DGW_(TS,CD,T,N, height, FORK_HEIGHT, difficulty_guess);  }

		// CD gets 1 block ahead of TS
	  CD.push_back(CD.back() + next_D);
	  if (CD.size() > N+1 ) { CD.erase(CD.begin()); }
	}
  // Simulate soolvetime this next_D
 

	simulated_ST = static_cast<u>(static_cast<float>(log( 1/fRand(0,1))*(CD.back()-CD[CD.size()-2])*DX)/HR);

	if (USE_CN_DELAY) {  current_ST = previous_ST; previous_ST = simulated_ST; }
  else { current_ST = simulated_ST; }
	// TS catches up with CD
	TS.push_back(TS.back() + current_ST); 
	if (TS.size() > N+1 ) { TS.erase(TS.begin()); }
	
	Ds[i]  = CD.back() - CD[CD.size()-2];
	HRs[i] = HR;	
	STs[i] = current_ST;
  avgST += current_ST;
	avgD  += Ds[i];
  avgHR += HR;
	// cout << i << " TS:" << TS.back() << " ST: " << STs[i] << " D: " << Ds[i] << endl; 
	
	// if (Ds[i] < 100 ) { cout << " D went < 100 at iteration " << i << " " << Ds[i] << endl; }
}
avgST /= BLOCKS; avgHR /= BLOCKS; avgD /= BLOCKS;
 
cout << " avgST: " << avgST << " avgD: " << avgD << " " << DA << endl;

string temp = "blocks_" + DA + ".txt";	ofstream file(temp);


vector<float>nD(BLOCKS), nST(BLOCKS), nHR(BLOCKS),nST11(BLOCKS),nAttack(BLOCKS);

float SD(0);
for (i=0;i<BLOCKS;i++) {	
	nD[i] = round(Ds[i]*100/avgD)/100;
	nHR[i] = round(HRs[i]*100/baseline_HR)/100;
	nST[i] = round(STs[i]*100/T)/100;
	// if (nST[i] > 8 ) { cout << " ST > 8xT " << nST[i] << endl; }

  if (i >= 5 && i < BLOCKS-5) { 
			for (int j = i-5; j <= i+5; j++) { nST11[i] += STs[j]; };
			nAttack[i] = (11*T)/(nST11[i]+1);
			nST11[i]   = round((nST11[i]*100)/(T*11))/100; 
			nAttack[i] = round(nAttack[i]*100)/100;
		//		nST11[i] = accumulate(nST[i-5],nST[+5])/11; nAttack[i]=1/nST11[i]; 
	
	}
  if (ENABLE_FILE_WRITES) {
		file << i+FORK_HEIGHT << "\t" << STs[i] << "\t" << Ds[i] << endl;
	}
	SD += (Ds[i]-avgD)*(Ds[i]-avgD)/BLOCKS/avgD/avgD;
}
file.close();
if (ENABLE_FILE_WRITES) {
	temp = "plot_" + DA + to_string(IDENTIFIER) + ".txt";	ofstream file2(temp);
	for (i=0;i<BLOCKS;i++) {	
		file2 << i+FORK_HEIGHT << "\t" << nD[i] << "\t" << nST[i] << "\t" << nHR[i] << "\t" << nST11[i] << "\t" << nAttack[i] << endl;
	}
	file2.close();
	int spacing = BLOCKS/60;
	int end = FORK_HEIGHT+BLOCKS;
 	temp = "gnuplot -c test_DAs_gnuplot.txt " + to_string(FORK_HEIGHT) + " " + to_string(end) + " " + 
			to_string(spacing) + " " + DA + to_string(IDENTIFIER) + " " + to_string(11) ;
	system((temp).c_str() );
	html_file << "<B>" << DA << "</B> Target ST/avgST= " << T << "/" << avgST << " N= " << N << " attack_size: " << 
         attack_size << " start/start: " << attack_start << "/" << attack_stop << " StdDev: " << sqrt(SD) <<
 				"\n<br><img src=gif_" << DA << to_string(IDENTIFIER) << ".gif><br>" << endl;
}
}
 
int main() 
{
//     sort(v.begin(), v.end()); 
srand(time(0)); // seed for fRand();

// These global constants are not typically changed for a given set of simulations.
// That's why they are global. 

BLOCKS = 5000; // number of BLOCKS to simulate
BASELINE_D = 10000; // The average D just before the fork. 
if (BASELINE_D < 10) { 
		cout << "BASELINE_D needs to be > 10 because of they way HR is used" << endl; return 0;
}
FORK_HEIGHT = 0; // if 0, then pretend it is genesis.
START_TIMESTAMP = 1540000000; // 0 is fine, or 1540000000 for typical times
START_CD = 1E9; // Beginning cumulative_difficulty. 0 is fine. 
USE_CN_DELAY = 1; // 1 = yes, 0=no. CN coins have a delay on timestamps
DX = 1;  // DX=1 for CN coins.
// DX is the "difficulty multiplier" which is crucial for non-CN coins. 
// DX=1 for CN coins. It's conversion that makes the following true:
// HR = DX * D / T
// D is scaled down in non-CN coins by a powLimit or MaxTarget that has
// "leading zeros".  DX = 2*(leading_zeros), so you can find DX by:
// DX = HR/D*T from current HR and D for a coin because you know the answer
// is a mulitple of 2, 4, 8, 16, or 32...etc


// The following variables are passed to the simulation and may not need changing.
// But often you will want to change them between the simulations below.

u attack_start = 90; // 90 = 90% of baseline D.  
u attack_stop = 120; // 120 = 120% of baseline D.
//  NOTE: use attack size = 100 to turn off hash attacks.
u attack_size = 100; // HR multiple of baseline HR .
u difficulty_guess = BASELINE_D; 
u T= 100;
u baseline_HR = (BASELINE_D*DX)/T; // baseline_HR = (BASELINE_D*DX)/T; to match HR to initial D
if (baseline_HR < 2) { 
	cout << "baseline_D/T ratio is too small to simulate with an integer HR." << endl; 
	exit(0); 
}

u N;
string DA;

ENABLE_FILE_WRITES = 1;

// You probably don't want a plot if the run is > 10,000
if (BLOCKS > 10000) { ENABLE_FILE_WRITES = 0; }

if (ENABLE_FILE_WRITES) {
	html_file << "<HTML><head><title>Difficulty Plots</title></head><body>" << endl;
}

USE_CN_DELAY = 0;

DA = "EMA_";
N = 30; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size); 

USE_CN_DELAY = 1;

DA = "EMA_";
N = 30; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size); 

// The identifier is to give the DA a different suffix in case you use the same DA
// with different settings or conditions. 

DA = "DIGISHIELD_";
N = 17; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size); 

DA = "LWMA4_"; 
N = 60; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size); 

// return 0;
 
DA = "LWMA1_";
N = 60; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size); 


DA = "LWMA1_";
N = 60; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size); 

DA = "SMA_"; 
N = 45; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size); 

DA = "DIGISHIELD_impoved_"; // problems removed
N = 17; IDENTIFIER++;
run_simulation(DA, T, N,  difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size); 

DA = "DGW_"; // It's just a SMA_ with 1/3 and 3x limits on ST
N=24; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size); 

html_file.close();
} ;

