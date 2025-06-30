/* Copyright (c) 2020, 2021 by Zawy, MIT license.

Just compile and run this to see the results.

See https://github.com/zawy12/difficulty-algorithms/issues/58

Chain work does not correctly determine the number of hashes performed. Chain work is the 
sum of difficulties (multiplied by a scaling factor such as 2^32 for BTC). For a large 
number of blocks it's usually pretty accurate, but to be precise when determining a leading 
tip, the number of hashes for N blocks is 

Hashes = sum(difficulty)  * (N-1)/N

This is precise only at the moment the last block is solved.  

The factor (N-1)/N corrects an error caused by the exponential distribution of solvetimes 
having 70% more faster-than-average solvetimes than it does slower-than-average.  As more 
blocks are found (N is larger), some will have substantially longer solvetimes that cancel 
the effect of the fast solvetime luck. At N=100, the error is reduced to 1%. This correction 
is the result of the Erlang distribution of N exponential solvetimes. 

Sum(Difficulty) * (N-1)/N is perfect only if difficulty is constant.  

*/
#include <iostream> 
#include <vector>
#include <fstream> 
#include <bits/stdc++.h> // for vectors
#include <sstream>
#include <string> 
#include <math.h>  
using namespace std; 
typedef double d;

d TARGET_TIME=0;
d fRand(d fMin, d fMax) { d f=(d)rand() / RAND_MAX; return fMin+f*(fMax-fMin); }
d print_out (d work, d HR, d H, string name) {
		cout << work << ",  " << HR << " (" << int(10000*(HR  -H)/H)/100 << "% error),  " << name << "\n";
		return 0;
}

d run_simulation( long int TIPS, vector<d>D, vector<d>HR) {
	// Back of vector D is the difficulty that's not yet solved.
	d current_D = D.back(); 	D.pop_back();
	d current_HR = HR.back();  HR.pop_back();
	assert ( D.size() == HR.size() );

	d N = D.size(), sum_Ds=0, avg_HR=0, ST=0, avg_ST=0;
	d sum_ST=0, sum_actual_work=0, sum_TT_inv_HR=0; 
	d avg_Z_HR=0, avg_Z_work=0, actual_work=0, avg_actual_HR=0, avg_actual_work=0;
	d avg_ZZ_work=0, avg_ZZ_HR=0;
	d avg_actual_ZZ_work=0, avg_actual_ZZ_HR=0;
	d avg_chain_work_HR=0, harmonic_sum=0, avg_H_work=0, avg_H_HR=0; // Harmonic difficulties 
	d sum_TT_easiness=0, avg_TT_work=0, avg_TT_HR=0; // target*time 
	d sum_S_easiness=0, avg_S_easiness=0, avg_S_HR_ease=0; // entropy 
	d sum_harmonic_work=0, avg_harmonic_work=0, avg_harmonic_HR=0;
	d m_A_w=0, m_A_hr=0, m_TT_hr=0, sd_A_w=0, TT_w=0, sd_A_hr=0, sd_TT_hr=0;

	// Get avg solvetime to adjust D's to get same-age tips.
	for ( long int i=1; i <= TIPS ; i++) {
		sum_ST = 0;
		for (int j = 0; j < N; j++ ) {
			ST = D[j]/HR[j] * log(1/fRand(0,1)); 
			sum_ST			+= ST;
		}
		avg_ST += sum_ST / TIPS; // avg TIP ST
	}
	for (int j = 0; j < N; j++ ) { 	
		D[j]	= D[j]*TARGET_TIME/avg_ST; 
		sum_Ds	+= D[j];
	}
	current_D = current_D*TARGET_TIME/avg_ST;
	// Adjust D and HR so that sum_D = 5 without changing TARGET_TIME.
	for (int j = 0; j < N; j++)  {
		D[j]	= D[j]*5/sum_Ds;
		HR[j]	= HR[j]*5/sum_Ds;		
	}
	current_D = current_D*5/sum_Ds;
	current_HR = current_HR*5/sum_Ds;
	avg_ST=0;
	sum_Ds=0;
	
	cout <<"-----  Difficulties and Hashrates: -----\nD:  ";
	for ( int j=0; j<N; j++) {  
		cout << D[j] << ", ";
		sum_Ds += D[j];
	//	avg_HR += D[j]*HR[j]; // temporary calculation to get difficulty-weighted avg_HR. 
		harmonic_sum += 1/D[j];  
	}
	cout << " unsolved D: " << current_D;
	// avg_HR = avg_HR / sum_Ds;
	cout << "\nHR: ";
	for (int j=0; j<N; j++) { cout << HR[j] << ", "; }
	cout << " unsolved HR: " << current_HR;
	cout << "\n\n";
	
	// Need to get means for Std Dev calculations
	for (long int i=1; i <= TIPS ; i++) {
		sum_ST = 0;
		sum_actual_work = 0;
		sum_harmonic_work = 0;
		for (int j = 0; j < N; j++ ) {
			sum_ST += D[j]/HR[j] * log(1/fRand(0,1));
			sum_actual_work	+= ST*HR[j];  // This is an *observation* of hashes = D[j]*log(1/rand)
			sum_TT_easiness	+= ST/D[j];
		}
		// means of actual work and hashrates and TT
		m_A_w	+= sum_actual_work / TIPS; 
		m_A_hr	+= sum_actual_work / sum_ST / TIPS;
		m_TT_hr	+= N/sum_TT_easiness*(N-1)/N / TIPS;
	}

	for (long int i=1; i <= TIPS ; i++) {
		sum_ST = 0;
		sum_actual_work = 0;
		sum_TT_easiness = 0;
		sum_S_easiness = 0;
		sum_harmonic_work = 0;
		actual_work = 0;
		d final_ST = 0;
		d each_interval = 0;
		// The following sums are to get avgs PER TIP.
		// Avg ST != avg 1/ST for small samples due to exponential distribution
		// more likely to have fast solvetimes.
		// Expected avg_1M(avg_N(STs)) != avg_1M(1/avg_N(STs)). 
		for (int j = 0; j < N; j++ ) {
			ST = D[j]/HR[j] * log(1/fRand(0,1)); 
			// ST = std::max(0.002,double(ST)); // To simulate no < 1 sec solves with T=500
			sum_ST			+= ST;
			actual_work		+= ST*HR[j];  // Notice this is just going to be sum of D's
			sum_TT_easiness += ST/D[j]; // TT = target*time. time*(P of finding a block) = 1/HR[j]*log(1/rand)
			sum_S_easiness	+= log(ST/D[j]);
		}
		final_ST 	= D[N-1]/HR[N-1] * log(1/fRand(0,1));		
		avg_actual_ZZ_work 	+= (actual_work + final_ST*HR[N-1])/TIPS;
		avg_actual_ZZ_HR 	+= (actual_work + final_ST*HR[N-1])/(sum_ST + final_ST)/TIPS;

		avg_ST				+= sum_ST/TIPS; 
		avg_actual_work		+= actual_work / TIPS; 
		avg_actual_HR		+= actual_work /sum_ST / TIPS;
		avg_chain_work_HR	+= sum_Ds/sum_ST / TIPS;	

		sd_A_w	+= (actual_work - m_A_w)*(actual_work - m_A_w) / TIPS;
		sd_A_hr	+= (actual_work/sum_ST - m_A_hr)*(actual_work/sum_ST - m_A_hr) / TIPS;

		TT_w	= N/sum_TT_easiness*(N-1)/N;
		sd_TT_hr	+= (TT_w/sum_ST - m_TT_hr)*(TT_w/sum_ST - m_TT_hr) / TIPS; 

		avg_Z_work		+= sum_Ds / TIPS; // for code symmetry
		avg_Z_HR		+= sum_Ds / sum_ST * (N-1)/N / TIPS ; 

		avg_ZZ_work 	+= sum_Ds / TIPS;			
		avg_ZZ_HR 		+= sum_Ds / (sum_ST+final_ST) / TIPS ;
			
		avg_H_work		+= N*N/harmonic_sum / TIPS; //  is smaller than sum Ds if Ds vary.
		avg_H_HR		+= N*N/harmonic_sum/sum_ST*(N-1)/N / TIPS;

		avg_TT_work		+= N/sum_TT_easiness * sum_ST / TIPS ;
		avg_TT_HR		+= N/sum_TT_easiness*(N-1)/N / TIPS;	

	}
	cout << int(N) << " blocks took " << avg_ST << " solvetimes.\n";
	cout << "A total of " << avg_ST << " solvetimes since split.\n";
	cout << "\n" << avg_actual_work << " (work), " <<  
		avg_actual_HR << " (HR) Input data" << "\nZZ actual work: " << avg_actual_ZZ_work << endl;	
	print_out(sum_Ds, avg_chain_work_HR,  avg_actual_HR,  "HR = (sum Ds)/(sum STs)");
	print_out(avg_Z_work, avg_Z_HR,  avg_actual_HR,  "HR = (sum Ds)/(sum STs)*(N-1)/N");
	print_out(avg_ZZ_work, avg_ZZ_HR, avg_actual_ZZ_HR, "ZZ HR = (sum Ds)/(final_ST + sum STs)");
	// print_out(avg_H_work, avg_H_HR,  avg_actual_HR,  "harmonic_mean_Ds/avg_STs *(N-1)/N incl. delay");
	print_out(avg_TT_work, avg_TT_HR,  avg_actual_HR,  "harmean(D/ST)*(N-1)/N  (Best HR, work is iffy)");
	
	cout << "\n"; 
	return 0;
}

int main() {
srand(time(0)); // seed fRand();
long int TIPS = 1e5; // how many "runs" to do
cout << fixed << setprecision(2);
cout << "Hashreate (HR) = 1 hash/(target solvetime)\nTarget solvetime = 1\n";
cout << "Difficulty = 2^256/target = avg # hashes to solution = 1/(Probability per hash)\n";

// TARGET_TIME is current time minus time of split. It is used to 
// adjust the difficulties so that all the tips have the same current time. 
// It will also adjust HRs so that chain work = 1. This allows
// much easier comparison between the different HR metrics.
// Setting this first run to all 1's for D and HR can set the 
// TARGET_TIME.

TARGET_TIME = 0;

// These difficulties & hashrates are scaled in the outputs for an historical reason.
vector<d>D  = {1,1,1, 1}; // Difficulties
vector<d>HR = {1,1,1, 1}; // Hashrate

for (int i=0; i<HR.size(); i++) { TARGET_TIME += D[i]/HR[i];  }

run_simulation(TIPS, D, HR);
D  = {1,1,2,2, 1}; 
HR = {1,1,1,1, 1};
run_simulation(TIPS, D, HR);
D  = {1,1,1,1, 1}; 
HR = {1,3,3,1, 1};
run_simulation(TIPS, D, HR);
D  = {1,1,1,1, 1}; 
HR = {2,2,2,2, 2};
run_simulation(TIPS, D, HR);
D  = {3,3,1,1,1}; 
HR = {2,2,1,1, 1};
run_simulation(TIPS, D, HR);
D  = {3,3,1,1, 1}; 
HR = {1,1,2,2, 1};
run_simulation(TIPS, D, HR);
exit(0);
}
