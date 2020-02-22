/* Copyright (c) 2020 by Zawy, MIT license.

Hashrate is a better metric than chain work for selecting a winning tip because we want the highest average hashrate. 
To everyone's surprise, this is not necessarily the tip with the highest chain work.  This is because the exponential 
distribution of solvetimes means fast solves are more likely.  Since solvetime is in the hashrate denominator, we can 
greatly overestimate hashrate if there are only a few samples, such as when comparing chain tips with less than 100 
blocks since their split.  

HR = hashrate
CW = chain work
D = difficulty
ST = solvetime

CW = (sum D)/(sum ST)
HR = (sum D)/(sum ST) * (N-1)/N. 

D is usually constant (or close to it), so 4 blocks will have 33% more CW than 3 blocks. The HR of the 4 blocks is 50% more.  
There are several assumptions in order for the comparison and HR equation to be correct:

1. The last timestamps are the same.
2. The last timestamps just occurred.
3. The timestamps are correct. (at least have tight timestamp limits).

The only adjustment to HR I have been able to think of that includes the "time since last block" works like this. 

> If a new block shows up now and it decreases the HR from the current estimate, then assume it just showed up and apply that adjustment."  

This means doing the following.
```
tslb = time since last block
current_D = difficulty of the block that has not been solved.

if ....
(sum N D)/(sum N ST)*(N-1)/N > (sum N D + current_D)/(sum N ST + tslb)*N/(N+1)
then ....
HR = (sum N D + current_D)/(sum N ST + tslb)*N/(N+1)
```

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
}
d run_simulation( long int TIPS, vector<d>D, vector<d>HR, d tslb) {
	// Back of vector D is the difficulty that's not yet solved.
	d current_D = D.back(); 	D.pop_back();
	d current_HR = HR.back();  HR.pop_back();
	assert ( D.size() == HR.size() );

	d N = D.size(), sum_Ds=0, avg_HR=0, ST=0, avg_ST=0;
	d sum_ST=0, sum_actual_work=0, sum_TT_inv_HR=0; 
	d avg_Z_HR=0, avg_Z_work=0, avg_actual_HR=0, avg_actual_work=0;
	d avg_chain_work_HR=0;

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
		D[j] = D[j]*TARGET_TIME/(avg_ST+tslb); 
		sum_Ds += D[j];
	}
	current_D = current_D*TARGET_TIME/(avg_ST+tslb);
	// Adjust D and HR so that sum_D=5 without changing TARGET_TIME.
	for (int j = 0; j < N; j++)  {
		D[j] = D[j]*5/sum_Ds;
		HR[j] = HR[j]*5/sum_Ds;		
	}
	current_D = current_D*5/sum_Ds;
	current_HR = current_HR*5/sum_Ds;
	avg_ST=0;
	sum_Ds=0;
	
	cout <<"-----  Difficulties and Hashrates: -----\nD:  ";
	for ( int j=0; j<N; j++) {  
		cout << D[j] << ", ";
		sum_Ds += D[j];
		avg_HR += D[j]*HR[j];
	}
	cout << " unsolved D: " << current_D;
	avg_HR = avg_HR / sum_Ds;
	cout << "\nHR: ";
	for (int j=0; j<N; j++) { cout << HR[j] << ", "; }
	cout << " unsolved HR: " << current_HR;
	cout << "\n\n";
	
	for (long int i=1; i <= TIPS ; i++) {
		sum_ST = 0;
		sum_actual_work = 0;
		// The following sums are to get avgs PER TIP.
		// Avg ST != avg 1/ST for small samples due to exponential distribution
		// more likely to have fast solvetimes.
		// Expected avg_1M(avg_N(STs)) != avg_1M(1/avg_N(STs)). 
		for (int j = 0; j < N; j++ ) {
			ST = D[j]/HR[j] * log(1/fRand(0,1)); 
			// ST = std::max(0.002,double(ST)); // To simulate no < 1 sec solves with T=500
			sum_ST			+= ST;
			sum_actual_work	+= ST*HR[j];  // This is an *observation* of hashes = D[j]*log(1/rand)
		}
		avg_ST += sum_ST/TIPS; // without tslb
		avg_actual_work	+= (sum_actual_work + tslb*current_HR)/ TIPS; 
		avg_actual_HR	+= (sum_actual_work + tslb*current_HR)/(sum_ST+tslb) / TIPS;
		avg_chain_work_HR += sum_Ds/sum_ST / TIPS;	// no tslb adjustment

		// This is a HR adustment if it's been a long time since last block passed by.
		// If the time since last block is such that a N+1 block arriving now 
		// would reduce the HR, then apply that time, difficulty, & N+1 as if a block 
		// just arrived. Can be ignored if tslb = 0. Suggestion: set tslb = 0.
		if (tslb > sum_ST*(N*N/(N*N-1)/sum_Ds*(sum_Ds+current_D) -1) ) {

			avg_Z_work		+= (sum_Ds + current_D) / TIPS; 
			avg_Z_HR		+= (sum_Ds + current_D)/(sum_ST+tslb)*((N+1)-1)/(N+1)/ TIPS;
		}
		else { 
			avg_Z_work		+= sum_Ds / TIPS; 
			avg_Z_HR		+= sum_Ds / sum_ST * (N-1)/N / TIPS ;  
		 }

	}
	cout << int(N) << " blocks took " << avg_ST << " solvetimes.\n";
	cout << "Time since last block: " << tslb << " solvetimes.\n";
	cout << "A total of " << avg_ST+tslb << " solvetimes since split.\n";
	if (tslb*current_HR/current_D > avg_ST*avg_actual_HR/avg_actual_work) { 
		cout << "\nInterpretation warning: Last block delay is > 50% unlikely.\nActual values are not averages for the given HR.\n"; 
	}
	cout << "\n" << avg_actual_work << " (work), " <<  
		avg_actual_HR << " (HR) <== observation incl. delay" << "\n\n";	
	print_out(sum_Ds, avg_chain_work_HR,   avg_actual_HR,  "HR = (sum Ds)/(sum STs)");
	print_out(avg_Z_work, avg_Z_HR,  avg_actual_HR,  "HR = (sum Ds)/(sum STs)*(N-1)/N incl. delay");

	cout << "\n";
}

int main() {

srand(time(0)); // seed fRand();

long int TIPS = 1e6; // how many "runs" to do

cout << "Hashreate (HR) = 1 hash/(target solvetime)\nTarget solvetime = 1\n\n";

// Difficulty = 2^256/target = avg # hashes to solution = 1/(Probability per hash)

cout << "Work (% error), Hashrate (% error)\n\n";

// Last element of vector D and HR are an unsolved block

cout << fixed << setprecision(2);

// Set variables for runs
// LAST column is the curent block that is NOT solved.
vector<d>D  = {1,1,1,   1}; // Difficulties
vector<d>HR = {1,1,1,   1}; // Hashrate

// Keep the following tslb=0 if you don't want to deal with the
// complex interpretation that must be for delays since last found block.
d tslb = 0; // Time Since Last Block as fraction of target solvetime

// TARGET_TIME is current time minus time of split. It is used to 
// adjust the difficulties so that all the tips have the same current time. 
// It will also adjust HRs so that chain work = 1. This allows
// much easier comparison between the different HR metrics.
// Setting this first run to all 1's for D and HR can set the 
// TARGET_TIME.
TARGET_TIME = 0;
for (int i=0; i<HR.size(); i++) { TARGET_TIME += D[i]/HR[i];  }


run_simulation(TIPS, D, HR, tslb);

D  = {1,1,1,1, 1}; 
HR = {1,1,1,1, 1};
run_simulation(TIPS, D, HR, tslb);

D  = {1,1,1,1, 1}; 
HR = {2,2,2,2, 2};
run_simulation(TIPS, D, HR, tslb);
D  = {2,2,2,2, 2}; 
HR = {1,1,1,1, 1};
run_simulation(TIPS, D, HR, tslb);

D  = {1,1,2,2, 1}; 
HR = {1,1,1,1, 1};
run_simulation(TIPS, D, HR, tslb);

D  = {1,1,1,1, 1}; 
HR = {1,1,2,2, 1};
run_simulation(TIPS, D, HR, tslb);

exit(0);



}
