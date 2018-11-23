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

u blocks, fork_height, start_timestamp, start_CD, baseline_D, use_CN_delay,Dx; 


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
difficulty_type SMA(std::vector<uint64_t> timestamps, 
   std::vector<uint64_t> cumulative_difficulties, uint64_t T, uint64_t N, uint64_t height,
					uint64_t fork_height,uint64_t difficulty_guess) {
   // Genesis should be the only time sizes are < N+1.
   assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );

   // Hard code D if there are not at least N+1 blocks after fork (or genesis)
   if (height < fork_height + N) { return difficulty_guess; }
   assert(timestamps.size() == N+1); 
	uint64_t ST = std::max(N,timestamps[N] - timestamps[0]);
	return uint64_t (cumulative_difficulties[N]-cumulative_difficulties[0])*T / ST;
}
// Dark Gravity Wave
// DGW3 uses an insanely complicated loop that is a simple moving average with double
// weight given to the most recent difficulty, which has virtually no effect.  So this 
// is just a SMA with the 1/3 and 3x limits in DGW3.
difficulty_type DGW3(std::vector<uint64_t> timestamps, 
   std::vector<uint64_t> cumulative_difficulties, uint64_t T, uint64_t N, uint64_t height,
					uint64_t fork_height,uint64_t  difficulty_guess) {
   // Genesis should be the only time sizes are < N+1.
   assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );

   // Hard code D if there are not at least N+1 blocks after fork (or genesis)
   if (height < fork_height + N) { return difficulty_guess; }
   assert(timestamps.size() == N+1); 

	uint64_t ST = std::max(N,timestamps[N] - timestamps[0]);
  ST = std::max((N*T)/3,std::min((N*T)*3,ST));
	return uint64_t (cumulative_difficulties[N]-cumulative_difficulties[0])*T / ST;
}

// Digishield
// Digishield uses the median of past 11 timestamps as the beginning and end of the window.
// This only simulates that, assuming the timestamps are always in the correct order.
// Also, these results were 
difficulty_type digishield(std::vector<uint64_t> timestamps, 
   std::vector<uint64_t> cumulative_difficulties, uint64_t T, uint64_t N, uint64_t height,
					uint64_t fork_height,uint64_t  difficulty_guess) {

   // Genesis should be the only time sizes are < N+1.
   assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );

   // Hard code D if there are not at least N+1 blocks after fork (or genesis)
   if (height < fork_height + N) { return difficulty_guess; }
   assert(timestamps.size() == N+1); 

	uint64_t ST = std::max(N,timestamps[N-6] - timestamps[0]);
  ST = std::max(((N-6)*T)*84/100,std::min((N-6)*T*132/100,ST));
	return uint64_t (cumulative_difficulties[N]-cumulative_difficulties[6])*T*400/(300*(N-6)*T+99*ST);
}

// Digishield without MTP=11 delay and no limits
difficulty_type digishield_improved(std::vector<uint64_t> timestamps, 
   std::vector<uint64_t> cumulative_difficulties, uint64_t T, uint64_t N, uint64_t height,
					uint64_t fork_height,uint64_t  difficulty_guess) {
   // Genesis should be the only time sizes are < N+1.
   assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );

   // Hard code D if there are not at least N+1 blocks after fork (or genesis)
   if (height < fork_height + N) { return difficulty_guess; }
   assert(timestamps.size() == N+1); 

	uint64_t ST = std::max(N,timestamps[N] - timestamps[0]);
	return uint64_t (cumulative_difficulties[N]-cumulative_difficulties[0])*T*400/(300*N*T+100*ST);
}

//  EMA difficulty algorithm
difficulty_type EMA(std::vector<uint64_t> timestamps, 
   std::vector<uint64_t> cumulative_difficulties, uint64_t T, uint64_t N, uint64_t height,
					uint64_t fork_height,uint64_t  difficulty_guess) {
  // The last 3 input variables above are not used. They're their for consistency with the other DAs.

	// Safely prevent out of sequence timestamps.
	uint64_t previous_timestamp(0), this_timestamp(0), ST, i;
	previous_timestamp = timestamps.front()-T;
	for ( i = 0; i < timestamps.size(); i++) {
  	 if ( timestamps[i] > previous_timestamp ) { this_timestamp = timestamps[i]; } 
  	 else { this_timestamp = previous_timestamp; }
     ST = std::max(1+T/50,std::min(8*T,this_timestamp - previous_timestamp));
  	 previous_timestamp = this_timestamp; 
	}
 
	/* Calculate e^(t/T)*1000 using integer math w/o pow() to
	ensure cross-platform/compiler consistency.
	This approximation comes from
	e^x = e^(floor(x)) * ef  
	where ef = 1 + f + f*f/2 + f*f*f/6 = 1+f(1+f(1+f/3)/2) 
	where f=1 for mod(x)=0 else f=mod(x) where x = t/T
*/
	uint64_t s = 10000;
	uint64_t exk = s; // exk = e^x * 1000
	for ( int i = 1; i <= ST/T/N ; i++ ) { 	exk = (exk*27183)/s; 	} // 27183 must be same # digitits as s
	uint64_t tm = ST % (T*N); 
	exk = (exk*(s+(tm*(s+(tm*(s+(tm*(s+(tm*s)/(4*T*N)))/(3*T*N)))/(2*T*N)))/(T*N)))/s;
	uint64_t prev_D = cumulative_difficulties.back() - cumulative_difficulties[cumulative_difficulties.size()-2];
	uint64_t next_D = ((s+s/1000)*prev_D*ST)/(s*T+(ST-T)*exk); 
	// For non-small N, this works as well as all the exk mess.
  // next_D = (prev_D*N*T)/(T*N+ST-T); // TH method who's denominator can't go negative.
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

difficulty_type LWMA1(std::vector<uint64_t> timestamps, 
   std::vector<uint64_t> cumulative_difficulties, uint64_t T, uint64_t N, uint64_t height,  
					uint64_t fork_height,uint64_t  difficulty_guess) {
    
   // This old way was not very proper
   // uint64_t  T = DIFFICULTY_TARGET_V2;
   // uint64_t  N = DIFFICULTY_WINDOW_V2; // N=60, 90, and 120 for T=600, 120, 60.
   
   // Genesis should be the only time sizes are < N+1.
   assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );

   // Hard code D if there are not at least N+1 blocks after fork (or genesis)
   if (height < fork_height + N) { return difficulty_guess; }
   assert(timestamps.size() == N+1); 
   

   uint64_t  L(0), next_D, i, this_timestamp(0), previous_timestamp(0), avg_D;
   // If hashrate/difficulty ratio after a fork is < 1/3 prior ratio, hardcode D for N+1 blocks after fork. 
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
	// Make least 2 digits = size of hash rate change last 11 blocks if it's statistically significant.
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

difficulty_type LWMA4(std::vector<uint64_t> timestamps, 
   std::vector<difficulty_type> cumulative_difficulties, uint64_t T, uint64_t N, uint64_t height,  
					uint64_t fork_height,uint64_t  difficulty_guess) {
    
   // This old way was not very proper
   // uint64_t  T = DIFFICULTY_TARGET_V2;
   // uint64_t  N = DIFFICULTY_WINDOW_V2; // N=60, 90, and 120 for T=600, 120, 60.

    
   // Genesis should be the only time sizes are < N+1.
   assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );

   // Hard code D if there are not at least N+1 blocks after fork (or genesis)
   if (height < fork_height + N) { return difficulty_guess; }
   assert(timestamps.size() == N+1); 

   uint64_t  L(0), ST(0), next_D, prev_D, avg_D, i;
        
   
   // If hashrate/difficulty ratio after a fork is < 1/3 prior ratio, hardcode D for N+1 blocks after fork. 
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

int run_simulation(string DA, u T, u N,u difficulty_guess,u initial_HR,u attack_start,u attack_stop,u attack_size) {

if ( DA == "digishield" ) { N=N+6; } // For simulated MTP = 11 delay.

	// In theory, this just uses ST = -ln(rand())* T * D/HR, but it's a mess, half-way because of CN_delay.
  // I should have made two different methods.  One with CN delay and one without.
  // Even without that it would have been a mess because of genesis or fork option.  Needed 4 methods.

	u next_D, simulated_ST(0), i, HR(0), attack_on(0);
	float avgST(0), avgHR(0), avgD(0);
	u previous_ST, current_ST;
	vector<u>STs(blocks);
  vector<u>Ds(blocks);
	vector<u>HRs(blocks);

	// Initially assume it's genesis
	vector<u>TS(1);
	TS[0] = start_timestamp;

	vector<u>CD(1);
	CD[0] = start_CD; 

	// If it's a fork initialize previous N blocks
	if (fork_height >= N+1) {
		TS.resize(N+1);
		CD.resize(N+1);
		// Note that TS[N] is not stored. CD must be 1 block ahead because
		// TS[N] depends on CD[N] (unless CN delay) in which case TS[N+1] depends on CD[N].
		for (i=1; i<=N; i++) { 	
			TS[i] = TS[i-1]+T;
			CD[i] = CD[i-1]+baseline_D; 
		} 
	}
	else if (fork_height == 0 ) { 
		TS.push_back(TS[0]+T); 
		CD.push_back(CD[0]+baseline_D);
	}
	else if (fork_height > 1 ) {
		cout << "You can't fork if genesis was < N+1 blocks in the past." << endl; 
	}

// *****  Run simulation  ********
previous_ST = T; // initialize in case we are using CN delay.
HR = initial_HR;
u height = fork_height; // This is kind of dummy work to send DA's what they need.
for (i=0; i <= blocks-1; i++) {
	// attack_size = 0 means there's no hash attack, but just constant HR.
	if (next_D > (attack_stop*baseline_D)/100  ) { 
		attack_on = 0;
		HR = baseline_D*Dx/T;
	}
	else if (next_D < (attack_start*baseline_D)/100 ) {
		attack_on = 1;
		HR = (baseline_D*Dx*attack_size)/(T*100);
	}
  // Run DA
	height = fork_height+i;
  if ( i < blocks-1 ) { 

		if (DA == "LWMA1" ) { next_D = LWMA1(TS,CD,T,N, height, fork_height, difficulty_guess);  }
		if (DA == "LWMA4" ) { next_D = LWMA4(TS,CD,T,N,height,fork_height, difficulty_guess);  }
		if (DA == "digishield" ) { next_D = digishield(TS,CD,T,N, height, fork_height, difficulty_guess);  }  
		if (DA == "digishield_improved" ) { next_D = digishield_improved(TS,CD,T,N,height,fork_height, difficulty_guess);  } 
		if (DA == "SMA" ) { next_D = SMA(TS,CD,T,N, height, fork_height, difficulty_guess);  }
		if (DA == "EMA" ) { next_D = EMA(TS,CD,T,N, height, fork_height, difficulty_guess);  }
		if (DA == "DGW3" ) { next_D = DGW3(TS,CD,T,N, height, fork_height, difficulty_guess);  }

		// CD gets 1 block ahead of TS
	  CD.push_back(CD.back() + next_D);
	  if (CD.size() > N+1 ) { CD.erase(CD.begin()); }
	}
  // Simulate soolvetime this next_D
	simulated_ST = static_cast<u>(static_cast<float>(0.5+(CD.back()-CD[CD.size()-2])*Dx/HR*log( 1/fRand(0,1) )));

	if (use_CN_delay) {  current_ST = previous_ST; previous_ST = simulated_ST; }
  else { current_ST = simulated_ST; }
	// TS catches up with CD
	TS.push_back(TS.back() + current_ST); 
	if (TS.size() > N+1 ) { TS.erase(TS.begin()); }
	// For testing.
	
	 Ds[i]  = CD.back() - CD[CD.size()-2];
	 HRs[i] = HR;	
	 STs[i] = current_ST;
  avgST += current_ST;
	avgD  += Ds[i];
  avgHR += HR;
 if (DA == "SMAx" ) { cout << i << " " << TS.back() << " " << STs[i] << " " << CD.back() << " " << Ds[i] << " " << next_D << endl; }
}
avgST /= blocks; avgHR /= blocks; avgD /= blocks;
cout << " avgST: " << avgST << " avgD: " << avgD << " " << DA << endl;

string temp = DA + "_test_DAs_blocks.txt";	ofstream file(temp);
float SD(0);
vector<float>nD(blocks), nST(blocks), nHR(blocks);
for (i=0;i<blocks;i++) {	
	nD[i] = Ds[i]/avgD;
	nHR[i] = HRs[i]/avgHR;
	nST[i] = STs[i]/avgST;

	file << i+fork_height << "\t" << STs[i] << "\t" << Ds[i] << endl;
	SD += (Ds[i]-avgD)*(Ds[i]-avgD);
}
file.close();

temp = DA + "_test_DAs_plot.txt";	ofstream file2(temp);
for (i=0;i<blocks;i++) {	
	file2 << i+fork_height << "\t" << nD[i] << "\t" << nST[i] << "\t" << nHR[i] << endl;
}
file2.close();

int spacing = blocks/60;
int end = fork_height+blocks;
 temp = "gnuplot -c test_DAs_gnuplot.txt " + to_string(fork_height) + " " + to_string(end) + " " + to_string(spacing) + " " + DA + " " + to_string(blocks) ;
system((temp).c_str() );
html_file << "<B>" << DA << "</B> Target ST/avgST= " << T << "/" << avgST << " N= " << N << " attack_size: " << 
          attack_size << " start/start: " << attack_start << "/" << attack_stop << "\n<br><img src=" << DA << ".gif><br>" << endl;
}

int main() 
{
//     sort(v.begin(), v.end()); 
srand(time(0)); // seed for fRand();

// These global constants are not typically changed for a given set of simulations.
// That's why they are global. 

blocks = 10000; // number of blocks to simulate
baseline_D = 1000; // The average D just before the fork. 
if (baseline_D < 1000) { cout << "baseline_D needs to be > 999 because of they way HR is used" << endl; return 0;}
fork_height = 300; // if 0, then pretend it is genesis.
start_timestamp = 1540000000; // 0 is fine, or 1540000000 for typical times
start_CD = 1E9; // Beginning cumulative_difficulty. 0 is fine. 
use_CN_delay = 0; // 1 = yes, 0=no.
Dx = 1; 
// Dx is the "difficulty multiplier" which is crucial for non-CN coins. 
// Dx=1 for CN coins. It's conversion that makes the following true:
// HR = Dx * D / T
// D is scaled down in non-CN coins by a powLimit or MaxTarget that has
// "leading zeros".  Dx = 2*(leading_zeros), so you can find Dx by:
// Dx = HR/D*T from current HR and D for a coin because you know the answer
// is a mulitple of 2, 4, 8, 16, or 32...etc


// The following variables are passed to the simulation and may not need changing.

u attack_start = 90; // 90 = 90% of baseline D.  
u attack_stop = 110; // 120 = 120% of baseline D.
//  NOTE: use attack size = 100 to turn off hash attacks.
u attack_size = 300; // HR multiple of baseline HR which is baseline_D/T, 300 = 3x.
u difficulty_guess = baseline_D; 
u T= 120;
u initial_HR = (baseline_D*Dx)/T;
u N;
string DA;


html_file << "<HTML><head><title>Difficulty Plots</title></head><body>" << endl;

DA = "LWMA1";
N = 90;
run_simulation(DA, T, N, difficulty_guess, initial_HR, attack_start,attack_stop,attack_size); 

DA = "LWMA4";
N = 90;
run_simulation(DA, T, N, difficulty_guess, initial_HR, attack_start,attack_stop,attack_size); 

DA = "EMA";
N = 50;
run_simulation(DA, T, N, difficulty_guess, initial_HR, attack_start,attack_stop,attack_size); 

DA = "SMA"; 
N = 45;
run_simulation(DA, T, N, difficulty_guess, initial_HR, attack_start,attack_stop,attack_size); 

DA = "digishield";
N = 17;
run_simulation(DA, T, N, difficulty_guess, initial_HR, attack_start,attack_stop,attack_size); 

DA = "digishield_improved"; // problems removed
N = 17;
run_simulation(DA, T, N,  difficulty_guess, initial_HR, attack_start,attack_stop,attack_size); 

DA = "DGW3"; // It's just a SMA with 1/3 and 3x limits on ST
N=24;
run_simulation(DA, T, N, difficulty_guess, initial_HR, attack_start,attack_stop,attack_size); 
html_file.close();
} ;

