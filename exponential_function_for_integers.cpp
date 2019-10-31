#include <iostream>     // for cout & endl
#include <math.h>		// for exp, pow, abs

// Exponential Function for Integers
// Copyright (c) Zawy 2019
// MIT License

uint64_t exponential_function_for_integers (uint64_t x_times_10k) {

	// This calculates e^x without decimals by passing it an integer x_times_10k and getting 
	// in return 10,000*e^(x_times_1k/10,00). e^24,9999 (x=24.9999) is highest value it accepts. 
	// Error is < +/-0.015%, avg = +/- 0.005%.  The math: let x = N.nnnn so that e^x = e^N * e^0.nnnn because series expansion 
	// for e^0.nnnn is a lot more accurate than e^N values which can be more easily stored or calculated. 
	uint64_t k = 1e4, R=0;
	uint64_t exx = k;
	uint64_t exp_of_integers[25] = { 10000, 27183, 73891, 200855, 545982, 1484132, 4034288, 10966332, 29809580, 81030839, 
				220264658, 598741417, 1627547914, 4424133920, 12026042842, 32690173725, 88861105205, 241549527536, 
				656599691373, 1784823009632, 4851651954098, 13188157344832, 35849128461316, 97448034462489, 264891221298435 };
	// for (u i = 1; i <= x_times_10k/k; i++ ) { exx = (exx*27183)/k; } // loop method can replace the above if k is changed
	 
	exx = exp_of_integers[x_times_10k/k];
	R = x_times_10k % k; // remainder 
	exx = (exx*(k+(R*(k+(R*(k+(R*(k+(R*(k+(R*(k+(R*k)/6/k))/5/k))/4/k))/3/k))/2/k))/k))/(k-1); // (k-1) fudge helped center error.
	if ( x_times_10k/k > 24 ) { exx = exp_of_integers[24]; }	
	return exx;
}

int main () {

//  The following simply demonstrates the above function's accuracy.

uint64_t k = 1e4, estimate_10k, j; 
double accurate, sumerr=0, err, mini=200000000,  maxi=-200000000, max=-200000000, i, minerror=11111, maxerror=11111, sqerr=0;

// generate list of exponent values.
// for (int p=0; p<25; p++) {  cout << static_cast<u>(exp(p)*k+.5) << ", "; } exit(0);

for (i = 0.002; i< 25; i = i+.002 ) {
	accurate = std::exp(i);
	estimate_10k = exponential_function_for_integers(static_cast<uint64_t>(k*i));
	// cout << "e^" << i << "   10k estimate: " << estimate_10k << "   accurate: " << accurate << endl;
	err = 100*((estimate_10k/accurate)/k - 1);
	if ( err > max) { max = err; maxerror=i; }
	if ( err < mini) { mini = err; minerror = i; }
	std::cout << err << "% error at e^" << i << std::endl;
	sqerr += err*err;
	j++;
	sumerr +=abs(err);
}
sqerr /= (j-1);
sqerr = pow(sqerr,0.5);
std::cout << "\n" << sumerr/j << " avg error. \n" << sqerr << " Std Dev.\n" << mini << "% min error.\n" << 
max << "% max error.\nMin error found at: e^" << minerror << "\nMax error found at: e^" << maxerror << std::endl;

return(0);
}