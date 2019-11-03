
/*
Copyright (c) 2018 by Zawy, MIT license.
Tests various difficulty algorithms for Cryptonote-type coins.
It's complicated because it's very flexible.
See main() to select algorithm(s) and settings.  
It can simulate on-off mining. 
It outputs timestamps and difficulties to screen and file.
Compile and running with 
g++ -std=c++11 test_DAs.cpp -o test_DAs && ./test_DAs
*/


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

// Allow some global constants for all simulations.
// These are set in main() so that there's only 1 place to change variables

u BLOCKS, FORK_HEIGHT, START_TIMESTAMP, START_CD, BASELINE_D, USE_CN_DELAY, DX, ENABLE_FILE_WRITES, PRINT_BLOCKS_TO_COMMAND_LINE, CONSTANT_HR(0); 
u IDENTIFIER(0);
int64_t R=4;

float fRand(float fMin, float fMax) {   
		float f = (float)rand() / RAND_MAX;
    return fMin + f * (fMax - fMin);
}

u TSA_simulate_ST(u T, int64_t R) {
   // simulates a time-to-solve 0 to 100 (% of T) for N=4 under either 
   // linear HR increase with D or constant
	u x = fRand(0,1000);
// Vectors based on x = e^(c*d) where c is HR function (1 or 0.5*(1+t/T) and d = (1-t/T)*e^(t/T/M)-1
// This was needed because I need t/T and can't solve these equations for it. 
vector<u>TSA_x_constant_HR_N_4{992,985,977,970,962,955,947,940,933,925,918,911,903,896,889,881,874,867,860,853,845,838,831,824,817,810,803,796,789,782,775,768,761,754,747,741,734,727,720,713,707,700,693,687,680,674,667,661,654,648,641,635,629,622,616,610,604,597,591,585,579,573,567,561,555,549,543,537,531,525,520,514,508,503,497,491,486,480,475,469,464,458,453,448,442,437,432,427,422,416,411,406,401,396,391,387,382,377,372,367,363,358,353,349,344,340,335,331,326,322,318,313,309,305,301,297,292,288,284,280,276,272,269,265,261,257,253,250,246,242,239,235,232,228,225,221,218,215,211,208,205,202,198,195,192,189,186,183,180,177,174,171,169,166,163,160,158,155,152,150,147,145,142,140,137,135,133,130,128,126,123,121,119,117,115,113,110,108,106,104,102,101,99,97,95,93,91,89,88,86,84,83,81,79,78,76,75,73,72,70,69,67,66,65,63,62,61,59,58,57,56,54,53,52,51,50,49,48,47,45,44,43,42,41,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0};
vector<u>TSA_t_constant_HR_N_4{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,234,235,236,237,239,240,241,243,244,246,248,249,251,253,255,257,259,261,263,266,269,271,274,278,281,284,289,294,300,305,313,323,338,350};
vector<u>TSA_x_linear_HR_N_4{996,992,988,984,980,976,971,967,962,958,953,949,944,939,934,929,924,919,914,909,903,898,892,887,881,875,870,864,858,852,846,840,834,828,821,815,809,802,796,789,783,776,770,763,756,749,743,736,729,722,715,708,701,694,687,680,673,666,658,651,644,637,630,622,615,608,600,593,586,579,571,564,557,550,542,535,528,520,513,506,499,492,485,477,470,463,456,449,442,435,428,421,414,408,401,394,387,381,374,367,361,354,348,342,335,329,323,316,310,304,298,292,286,281,275,269,263,258,252,247,241,236,231,226,221,215,210,206,201,196,191,187,182,177,173,169,164,160,156,152,148,144,140,136,133,129,125,122,118,115,112,108,105,102,99,96,93,90,87,85,82,79,77,74,72,69,67,65,63,61,58,56,54,53,51,49,47,45,44,42,40,39,37,36,35,33,32,31,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,8,7,6,5,4,3,2,1,0};
/*
Copyright (c) 2018 by Zawy, MIT license.
Tests various difficulty algorithms for Cryptonote-type coins.
It's complicated because it's very flexible.
See main() to select algorithm(s) and settings.  
It can simulate on-off mining. 
It outputs timestamps and difficulties to screen and file.
Compile and running with 
g++ -std=c++11 test_DAs.cpp -o test_DAs && ./test_DAs
*/


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

// Allow some global constants for all simulations.
// These are set in main() so that there's only 1 place to change variables

u BLOCKS, FORK_HEIGHT, START_TIMESTAMP, START_CD, BASELINE_D, USE_CN_DELAY, DX, ENABLE_FILE_WRITES, PRINT_BLOCKS_TO_COMMAND_LINE, CONSTANT_HR(1), HR_NEW_METHOD(1); 
u IDENTIFIER(0);
uint64_t R=4;

float fRand(float fMin, float fMax) {   
		float f = (float)rand() / RAND_MAX;
    return fMin + f * (fMax - fMin);
}

u TSA_simulate_ST(u T, u R) {
   // simulates a time-to-solve 0 to 100 (% of T) for N=4 under either 
	u x = fRand(0,1000);
// Vectors based on x = e^(c*d) where c is HR function (1 or 0.5*(1+t/T) and d = (1-t/T)*e^(t/T/M)-1
// This was needed because I need t/T and can't solve these equations for it. 
vector<u>TSA_x_constant_HR_N_4{992,985,977,970,962,955,947,940,933,925,918,911,903,896,889,881,874,867,860,853,845,838,831,824,817,810,803,796,789,782,775,768,761,754,747,741,734,727,720,713,707,700,693,687,680,674,667,661,654,648,641,635,629,622,616,610,604,597,591,585,579,573,567,561,555,549,543,537,531,525,520,514,508,503,497,491,486,480,475,469,464,458,453,448,442,437,432,427,422,416,411,406,401,396,391,387,382,377,372,367,363,358,353,349,344,340,335,331,326,322,318,313,309,305,301,297,292,288,284,280,276,272,269,265,261,257,253,250,246,242,239,235,232,228,225,221,218,215,211,208,205,202,198,195,192,189,186,183,180,177,174,171,169,166,163,160,158,155,152,150,147,145,142,140,137,135,133,130,128,126,123,121,119,117,115,113,110,108,106,104,102,101,99,97,95,93,91,89,88,86,84,83,81,79,78,76,75,73,72,70,69,67,66,65,63,62,61,59,58,57,56,54,53,52,51,50,49,48,47,45,44,43,42,41,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0};
vector<u>TSA_t_constant_HR_N_4{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,234,235,236,237,239,240,241,243,244,246,248,249,251,253,255,257,259,261,263,266,269,271,274,278,281,284,289,294,300,305,313,323,338,350};
vector<u>TSA_x_linear_HR_N_4{996,992,988,984,980,976,971,967,962,958,953,949,944,939,934,929,924,919,914,909,903,898,892,887,881,875,870,864,858,852,846,840,834,828,821,815,809,802,796,789,783,776,770,763,756,749,743,736,729,722,715,708,701,694,687,680,673,666,658,651,644,637,630,622,615,608,600,593,586,579,571,564,557,550,542,535,528,520,513,506,499,492,485,477,470,463,456,449,442,435,428,421,414,408,401,394,387,381,374,367,361,354,348,342,335,329,323,316,310,304,298,292,286,281,275,269,263,258,252,247,241,236,231,226,221,215,210,206,201,196,191,187,182,177,173,169,164,160,156,152,148,144,140,136,133,129,125,122,118,115,112,108,105,102,99,96,93,90,87,85,82,79,77,74,72,69,67,65,63,61,58,56,54,53,51,49,47,45,44,42,40,39,37,36,35,33,32,31,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,8,7,6,5,4,3,2,1,0};
vector<u>TSA_t_linear_HR_N_4{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,203,204,206,207,209,210,212,214,216,218,219,222,225,229,234,240,247,256};
vector<u>TSA_x_constant_HR_N_2{994,989,984,979,974,969,963,958,953,947,942,936,930,925,919,913,908,902,896,890,884,878,872,866,860,854,848,842,835,829,823,817,810,804,797,791,785,778,772,765,758,752,745,739,732,725,719,712,705,699,692,685,678,672,665,658,651,644,638,631,624,617,610,603,597,590,583,576,569,563,556,549,542,536,529,522,515,509,502,495,489,482,475,469,462,456,449,443,436,430,423,417,411,404,398,392,386,380,373,367,361,355,349,343,338,332,326,320,314,309,303,298,292,287,281,276,271,265,260,255,250,245,240,235,230,225,221,216,211,207,202,198,193,189,184,180,176,172,168,164,160,156,152,148,145,141,138,134,131,127,124,121,117,114,111,108,105,102,99,96,94,91,88,86,83,80,78,76,73,71,69,67,64,62,60,58,56,55,53,51,49,47,46,44,43,41,40,38,37,35,34,33,32,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0};
vector<u>TSA_t_constant_HR_N_2{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,208,209,210,212,213,215,217,219,221,223,226,229,231,236,240,246,255,269};
vector<u>TSA_x_linear_HR_N_2{997,994,992,989,986,983,980,977,974,970,967,963,960,956,952,949,945,941,937,932,928,924,919,915,910,905,900,895,890,885,880,875,869,864,858,853,847,841,835,829,823,817,810,804,798,791,784,778,771,764,757,750,743,736,729,721,714,707,699,692,684,676,669,661,653,645,637,629,621,613,605,597,589,581,573,564,556,548,540,531,523,515,506,498,490,481,473,465,457,448,440,432,424,416,407,399,391,383,375,367,359,352,344,336,329,321,313,306,299,291,284,277,270,263,256,249,242,235,229,222,216,210,204,197,191,186,180,174,168,163,158,152,147,142,137,132,128,123,118,114,110,106,101,97,94,90,86,83,79,76,73,69,66,63,60,58,55,52,50,48,45,43,41,39,37,35,33,31,30,28,26,25,23,22,21,20,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0};
vector<u>TSA_t_linear_HR_N_2{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,185,186,187,189,191,193,195,198,200,204,210,222};
vector<u>TSA_x_constant_HR_N_1{1000,999,998,997,996,995,994,992,991,989,988,986,984,982,980,977,975,972,970,967,964,961,957,954,950,946,942,938,934,930,925,921,916,911,906,900,895,889,884,878,872,865,859,852,846,839,832,825,817,810,802,795,787,779,771,762,754,746,737,728,719,710,701,692,683,673,664,654,644,634,625,615,605,594,584,574,564,554,543,533,523,512,502,491,481,470,460,450,439,429,419,408,398,388,378,368,358,348,338,329,319,309,300,291,281,272,263,255,246,237,229,221,213,205,197,189,182,175,167,161,154,147,141,134,128,122,117,111,106,100,95,90,86,81,77,73,69,65,61,57,54,51,48,45,42,39,37,34,32,30,28,26,24,22,20,19,17,16,15,14,12,11,10,9,8,7,6,5,4,3,2,1,0};
vector<u>TSA_t_constant_HR_N_1{2,4,6,7,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,169,170,171,173,174,177,180,184,190,202};
vector<u>TSA_x_linear_HR_N_1{1000,999,998,997,996,995,994,993,992,991,989,988,986,985,983,981,979,977,975,973,970,968,965,962,959,956,952,949,945,942,938,933,929,925,920,915,910,905,900,894,889,883,877,870,864,857,850,843,836,829,821,813,805,797,788,780,771,762,753,743,734,724,714,704,694,684,673,662,652,641,629,618,607,595,584,572,560,549,537,525,513,501,489,476,464,452,440,428,416,404,392,380,368,356,344,333,321,310,299,288,277,266,255,245,235,225,215,205,196,187,178,169,160,152,144,136,129,122,115,108,101,95,89,84,78,73,68,63,59,54,50,47,43,40,36,33,31,28,26,23,21,19,17,16,14,13,11,10,9,8,7,6,5,4,3,3,2,1,0};
vector<u>TSA_t_linear_HR_N_1{3,6,9,10,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,161,162,164,166,168,173,184};

vector<u>TSA_x, TSA_t;
   if (R == 2 && CONSTANT_HR) { TSA_x = TSA_x_constant_HR_N_2; TSA_t = TSA_t_constant_HR_N_2;}
   else if (R == 2 && !CONSTANT_HR) { TSA_x = TSA_x_linear_HR_N_2; TSA_t = TSA_t_linear_HR_N_2;}
   else if (R == 4 && CONSTANT_HR) { TSA_x = TSA_x_constant_HR_N_4; TSA_t = TSA_t_constant_HR_N_4;}
   else if (R == 4 && !CONSTANT_HR) { TSA_x = TSA_x_linear_HR_N_4; TSA_t = TSA_t_linear_HR_N_4;}
   else if (R == 1 && CONSTANT_HR) { TSA_x = TSA_x_constant_HR_N_1; TSA_t = TSA_t_constant_HR_N_1;}
   else if (R == 1 && !CONSTANT_HR) { TSA_x = TSA_x_linear_HR_N_1; TSA_t = TSA_t_linear_HR_N_1;}
   else { cout << "Only M=1,2, and 4 are supported in TSA simulation." << endl; }

   for (int j=0; j < TSA_x.size(); j++ ) {  
      if ( x >= TSA_x[j] ) { return (TSA_t[j]*T);  }
   }
}

// The following is not currently used.
uint64_t solvetime_without_exploits (std::vector<uint64_t>timestamps, uint64_t T) {
	uint64_t previous_timestamp(0), this_timestamp(0), ST, i;
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
// =======  Simple Moving Average (SMA)   ========
// ===============================================
// This will have a slightly slow avg ST for small N because it is terms of D instead of hash target.
// Harmonic D could be used, but it's not hardly possible with integer math. To work in terms of 
// target, it would probably require 2^256 math which is way beyond uint64_t.
difficulty_type SMA_(std::vector<uint64_t> timestamps, 
   std::vector<uint64_t> cumulative_difficulties, uint64_t T, uint64_t N, uint64_t height,
					uint64_t FORK_HEIGHT,uint64_t difficulty_guess) {

   // If it's Genesis ...
   if (height <= N+1 ) { return difficulty_guess; }

   // Hard code D if there are not at least N+1 BLOCKS after fork (or genesis)
   if (height >= FORK_HEIGHT && height <= FORK_HEIGHT + N+1) { return difficulty_guess; }

   // Check to see if the timestamp and CD vectors are correct size.
   assert(timestamps.size() == N+1 && timestamps.size() == cumulative_difficulties.size()); 

	uint64_t ST = std::max(N,timestamps[N] - timestamps[0]);
	return uint64_t (cumulative_difficulties[N]-cumulative_difficulties[0])*T / ST;
}

// ===========================
// ==========   DGW   ========
// ===========================
// DGW_ uses an insanely complicated loop that is a simple moving average with double
// weight given to the most recent difficulty, which has virtually no effect.  So this 
// is just a SMA_ with the 1/3 and 3x limits in DGW_.
difficulty_type DGW_(std::vector<uint64_t> timestamps, 
   std::vector<uint64_t> cumulative_difficulties, uint64_t T, uint64_t N, uint64_t height,
					uint64_t FORK_HEIGHT,uint64_t  difficulty_guess) {
   // Genesis should be the only time sizes are < N+1.
   assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );

   // Hard code D if there are not at least N+1 BLOCKS after fork (or genesis)
   if (height >= FORK_HEIGHT && height <= FORK_HEIGHT + N+1) { return difficulty_guess; }
   assert(timestamps.size() == N+1); 

	uint64_t ST = std::max(N,timestamps[N] - timestamps[0]);
  ST = std::max((N*T)/3,std::min((N*T)*3,ST));
	return uint64_t (cumulative_difficulties[N]-cumulative_difficulties[0])*T / ST;
}

// ================================
// ==========  Digishield  ========
// ================================
// Digishield uses the median of past 11 timestamps as the beginning and end of the window.
// This only simulates that, assuming the timestamps are always in the correct order.
// Also, this is in terms of difficulty instead of target, so a "101/100" correction factor was used.
difficulty_type DIGISHIELD(std::vector<uint64_t> timestamps, 
   std::vector<uint64_t> cumulative_difficulties, uint64_t T, uint64_t N, uint64_t height,
					uint64_t FORK_HEIGHT,uint64_t  difficulty_guess) {

   // Genesis should be the only time sizes are < N+1.
   assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );

   // Hard code D if there are not at least N+1 BLOCKS after fork (or genesis)
  if (height >= FORK_HEIGHT && height <= FORK_HEIGHT + N+1) { return difficulty_guess; }
   assert(timestamps.size() == N+1); 

	uint64_t ST = std::max(N,timestamps[N-6] - timestamps[0]);
  // THe following is supposed to represent their limits on ST, but it did not work.
   ST = std::max((((N-6)*T)*84)/100,std::min(((N-6)*T*132)/100,(300*(N-6)*T+101*ST)/400));
	return uint64_t (cumulative_difficulties[N]-cumulative_difficulties[6])*T*400/(300*(N-6)*T+99*ST);
}

// ===========================================
// ==========   improved Digishield   ========
// ===========================================
difficulty_type DIGISHIELD_impoved_(std::vector<uint64_t> timestamps, 
   std::vector<uint64_t> cumulative_difficulties, uint64_t T, uint64_t N, uint64_t height,
					uint64_t FORK_HEIGHT,uint64_t  difficulty_guess) {
   // Genesis should be the only time sizes are < N+1.
   assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );

   // Hard code D if there are not at least N+1 BLOCKS after fork (or genesis)
   if (height >= FORK_HEIGHT && height <= FORK_HEIGHT + N+1) { return difficulty_guess; }
   assert(timestamps.size() == N+1); 

	uint64_t ST = std::max(N,timestamps[N] - timestamps[0]);
	return uint64_t (cumulative_difficulties[N]-cumulative_difficulties[0])*T*400/(300*N*T+100*ST);
}

// ==============================
// ==========   EMA   ===========
// ==============================
difficulty_type EMA_(std::vector<uint64_t> timestamps, 
   std::vector<uint64_t> cumulative_difficulties, uint64_t uT, uint64_t uN) {
   assert(uN > 1);
   	
	int64_t T=uT, N=uN, previous_timestamp(0), this_timestamp(0), ST, i;
    // Safely handle out-of-sequence timestamps
	// This is awkward because N defines a window of timestamps to review but 
	// EMA only uses previous timestamp
	previous_timestamp = timestamps.front()-T;
	for ( i = 0; i < timestamps.size(); i++) {
  	 if ( timestamps[i] <= previous_timestamp ) { this_timestamp = previous_timestamp+1; } 
  	 else { this_timestamp = timestamps[i]; }
     ST = std::min(7*T,this_timestamp - previous_timestamp);
  	 previous_timestamp = this_timestamp; 
	}

	/* Calculate e^(t/T)*1000 using integer math w/o pow() to
	ensure cross-platform/compiler consistency.
	This approximation comes from 	e^x = e^(floor(x)) * ef  
	where ef = 1 + f + f*f/2 + f*f*f/6 = 1+f(1+f(1+f/3)/2) 
	where f=1 for mod(x)=0 else f=mod(x) where x = t/T   */  
	// double STf = ST, Tf=T, Nf=N;
	// static_cast<int64_t>(k*pow(2.7182,STf/Nf/Tf))

	int64_t k = 1E6;
	int64_t exk = k; 
	
	for ( int i = 1; i <= ST/(T*N) ; i++ ) { exk = (exk*static_cast<int64_t>(2.718*k))/k; } 
 
	int64_t f = ST % (T*N); 
	exk = (exk*(k+(f*(k+(f*(k+(f*k)/(3*T*N)))/(2*T*N)))/(T*N)))/k;
	int64_t prev_D = cumulative_difficulties.back() - cumulative_difficulties[cumulative_difficulties.size()-2];
	int64_t next_D = (prev_D*((1000*(k*ST))/(k*T+(ST-T)*exk)))/1000;
	// cout << prev_D << endl;

  
   if ( N==2) { next_D = (next_D*95)/100; }
   else if ( N==3) { next_D = (next_D*99)/100; }

	return static_cast<uint64_t>(next_D);  	
}
// ==============================
// ==========   LWMA-1   ========
// ==============================
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
   if (height >= FORK_HEIGHT && height <= FORK_HEIGHT + N+1) { return difficulty_guess; }
   assert(timestamps.size() == N+1); 
   

   uint64_t  L(0), next_D, i, this_timestamp(0), previous_timestamp(0), avg_D;
	
	previous_timestamp = timestamps[0];
	for ( i = 1; i <= N; i++) {        
		// Safely prevent out-of-sequence timestamps
		if ( timestamps[i]  > previous_timestamp ) {   this_timestamp = timestamps[i];  } 
		else {  this_timestamp = previous_timestamp+1;   }
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
/*
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
*/
	return  next_D;
}
// ==============================
// ==========   LWMA-4   ========
// ==============================
// LWMA-4 difficulty algorithm 
// Copyright (c) 2017-2018 Zawy, MIT License
// https://github.com/zawy12/difficulty-algorithms/issues/3
// See commented version for explanations & required config file changes. Fix FTL and MTP!

difficulty_type LWMA4_(std::vector<uint64_t> timestamps, 
   std::vector<difficulty_type> cumulative_difficulties, uint64_t T, uint64_t N, uint64_t height,  
					uint64_t FORK_HEIGHT,uint64_t  difficulty_guess) {
    
   // Genesis should be the only time sizes are < N+1.
   assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );

  // Hard code D if there are not at least N+1 BLOCKS after fork (or genesis)
   if (height >= FORK_HEIGHT && height <= FORK_HEIGHT + N+1) { return difficulty_guess; }
   assert(timestamps.size() == N+1); 

   uint64_t  L(0), ST(0), next_D, prev_D, avg_D, i;
        

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
// ============================
// ========  WHR   ===========
// ============================
// This is a test to see if the original WT-144 of LWMA by Tom Harding that weighted
// hashrate (difficulty/solvetime) instead of solvetimes could be made as good as 
// or better than LWMA. Yes, it was as good, but not better.

difficulty_type WHR_(std::vector<uint64_t> timestamps, 
   std::vector<uint64_t> cumulative_difficulties, uint64_t T, uint64_t N, uint64_t height,  
					uint64_t FORK_HEIGHT,uint64_t  difficulty_guess) {

   // Genesis should be the only time sizes are < N+1.
   assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );

   assert(N%2 == 0); 

   // Hard code D if there are not at least N+1 BLOCKS after fork (or genesis)
   if (height >= FORK_HEIGHT && height <= FORK_HEIGHT + N+1) { return difficulty_guess; }
   assert(timestamps.size() == N+1); 

   uint64_t  L(0), next_D, i, this_timestamp(0), previous_timestamp(0), avg_D;

   uint64_t ts =0, WHR, tar1, tar2;
 
//	if (height % 2) { // The if-else statement was not as accurate.

		previous_timestamp=timestamps[0];
		for ( i = 2; i <= N; i+=2) {        
			// Safely prevent out-of-sequence timestamps
			if ( timestamps[i]  > previous_timestamp ) {   this_timestamp = timestamps[i];  } 
			else {  this_timestamp = previous_timestamp+1;   }
			ts = this_timestamp - previous_timestamp;
			previous_timestamp = this_timestamp; 
			tar1 = 1e13/(cumulative_difficulties[i] - cumulative_difficulties[i-1]);
			tar2 = 1e13/(cumulative_difficulties[i-1] - cumulative_difficulties[i-2]);
			WHR +=  (i*ts)/2 * (tar1 + tar2)/2 * 2/(2-1); // 1/2's are for ts & target avgs
		}
    
		 next_D = ((1e13*T*N)*(N/2+1))/WHR;  
//	}
//	else {next_D = cumulative_difficulties[N] - cumulative_difficulties[N-1]; }

	return  next_D;
}

// ============================
// ========   TSA   ===========
// ============================

difficulty_type TSA(std::vector<uint64_t> timestamps, 
      std::vector<uint64_t> cumulative_difficulties, uint64_t T, uint64_t N, uint64_t height,  
            uint64_t FORK_HEIGHT, uint64_t  difficulty_guess, uint64_t template_timestamp, uint64_t M ) {

   uint64_t  L(0), next_D, i, this_timestamp(0), previous_timestamp(0), avg_D;

   int64_t sumST(0);

   assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );
   // Hard code D if there are not at least N+1 BLOCKS after fork or genesis
   if (height >= FORK_HEIGHT && height <= FORK_HEIGHT + N+1) { return difficulty_guess; }
   assert(timestamps.size() == N+1); 
   previous_timestamp = timestamps[0]-T;
   for ( i = 1; i <= N; i++) {        
      // Safely prevent out-of-sequence timestamps
      if ( timestamps[i]  >= previous_timestamp ) {   this_timestamp = timestamps[i];  } 
      else {  this_timestamp = previous_timestamp+1;   }
      L +=  i*std::min(6*T ,this_timestamp - previous_timestamp);
      sumST +=std::min(6*T ,this_timestamp - previous_timestamp);
      previous_timestamp = this_timestamp; 
   }
   avg_D = ( cumulative_difficulties[N] - cumulative_difficulties[0] )/ N;

   // Prevent round off error for small D and overflow for large D.
   if (avg_D > 2000000*N*N*T) { next_D = (avg_D/(200*L))*(N*(N+1)*T*99);  }   
   else {    next_D = (avg_D*N*(N+1)*T*99)/(200*L);    }	

// LWMA is finished, now use its next_D and previous_timestamp 
// to get TSA's next_D.  I had to shift from unsigned to signed integers.
 //  assert( R > static_cast<int64_t>(1));

   // warning M is unsigned
   int64_t ST, j, f, TSA_D = next_D, Ts = T, k = 1E6, TM = Ts*M, exk = k;
   if ( M == 1) { Ts = (85*T)/100; }
   if ( M == 2) { Ts = (95*T)/100; }
   if ( M == 3) { Ts = (99*T)/100; }
   
   if (template_timestamp <= previous_timestamp ) {
      template_timestamp = previous_timestamp+1;
   }
   ST = std::min(static_cast<int64_t>(template_timestamp) - static_cast<int64_t>(previous_timestamp), 6*Ts);
   ST = (ST*sumST)/(T*N); // kalkan's idea
   for (i = 1; i <= ST/TM ; i++ ) { exk = (exk*static_cast<int64_t>(2.718*k))/k; } 
   f = ST % TM;    
   exk = (exk*(k+(f*(k+(f*(k+(f*k)/(3*TM)))/(2*TM)))/(TM)))/k;
   TSA_D = std::max(static_cast<int64_t>(10),(TSA_D*((1000*k*ST) / 
                 std::max(static_cast<int64_t>(1),(k*Ts+(ST-Ts)*exk))))/1000);
  // Make all insignificant digits zero for easy reading.
   j = 1e12;
   while (j > 1) { 
      if ( TSA_D > j*100 ) { TSA_D = ((TSA_D+j/2)/j)*j; break; }
      else { j /= 10; }
   }

   return static_cast<uint64_t>(TSA_D);  	
}

// ============================
// ========  Boris (Zcoin)   ===========
// ============================
difficulty_type Boris_(std::vector<uint64_t> timestamps, 
   std::vector<uint64_t> cumulative_difficulties, uint64_t uT, uint64_t uN, uint64_t height,  
					uint64_t FORK_HEIGHT,uint64_t  difficulty_guess) {
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
	if (j < 1000 && height % 1 == 0 ) { cout << "XXXXXXXX                    " << j << "    " << next_D << endl; }
	return next_D;
}
// ==============================================
//  =========   RUN SIMULATION  =================
// ==============================================

int run_simulation(string DA, u T, u N,u difficulty_guess,u baseline_HR,u attack_start,u attack_stop,u attack_size, u R) {

// R is used for EMA and TSA

if ( DA == "DIGISHIELD_" ) { N=N+6; } // For simulated MTP = 11 delay.

   // In theory, this just uses ST = -ln(rand())* T * D/HR, but it's a mess, half-way because of CN_delay.
   // I should have made two different methods.  One with CN delay and one without.
   // Even without that it would have been a mess because of genesis or fork option.  Needed 4 methods.

	u next_D(0), TSA_Din, simulated_ST(0), i, HR(0), attack_on(0);
	float avgST(0), avgHR(0), avgD(0);
	u previous_ST, current_ST;
	vector<u>STs(BLOCKS);
  vector<u>Ds(BLOCKS);
	vector<u>HRs(BLOCKS);
  vector<float>Douts(BLOCKS,0); // TSA will be the only 1 to change the all-0 values

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
		cout << "You can't fork if genesis was < N+1 BLOCKS in the past." << endl; 
	}
// *****  Run simulation  ********
previous_ST = T; // initialize in case we are using CN delay.
HR = baseline_HR;
u height = FORK_HEIGHT; // This is kind of dummy work to send DA's what they need.

vector<float>rand_values(BLOCKS);
for (i=0; i <= BLOCKS-1; i++) { rand_values[i]=log( 1/fRand(0,1)); }

for (i=0; i <= BLOCKS-1; i++) {
	// attack_size = 0 means there's no hash attack, but just constant HR.
	if (DA == "TSA" ) {
      next_D = LWMA1_(TS,CD,T,N, height, FORK_HEIGHT, difficulty_guess);
	}
    else {
	  if (next_D > (attack_stop*BASELINE_D)/100 ) { // old: 
		  attack_on = 0;
		  HR = baseline_HR;
	  }
	  else if (next_D < (attack_start*BASELINE_D)/100 ) { // old: (attack_start*BASELINE_D)/100
		  attack_on = 1;
		  HR = (baseline_HR*attack_size)/100;
	  }
    }
  // if (HR == 0) { HR=1; cout << "HR was zero, so it was changed to 1 to prevent 1/0." << endl; }
  // Run DA
  
	height = FORK_HEIGHT+i;

		if (DA == "LWMA1_" ) {	next_D = LWMA1_(TS,CD,T,N, height, FORK_HEIGHT, difficulty_guess);  }
		if (DA == "LWMA4_" ) {	next_D = LWMA4_(TS,CD,T,N,height,FORK_HEIGHT, difficulty_guess);  }
		if (DA == "WHR_" ) {	next_D = WHR_(TS,CD,T,N, height, FORK_HEIGHT, difficulty_guess);  }
		if (DA == "Boris_" ) {	next_D = Boris_(TS,CD,T,N,height,FORK_HEIGHT, difficulty_guess);  }
		if (DA == "DIGISHIELD_" ) { next_D = DIGISHIELD(TS,CD,T,N, height, FORK_HEIGHT, difficulty_guess);  }  
		if (DA == "DIGISHIELD_impoved_" ) { next_D = DIGISHIELD_impoved_(TS,CD,T,N,height,FORK_HEIGHT, difficulty_guess);  } 
		if (DA == "SMA_" ) {	next_D = SMA_(TS,CD,T,N, height, FORK_HEIGHT, difficulty_guess);  }
		if (DA == "EMA_" ) {	next_D = EMA_(TS,CD,T,N);  }
		if (DA == "DGW_" ) {	next_D = DGW_(TS,CD,T,N, height, FORK_HEIGHT, difficulty_guess);  }
		//  **** Begin TSA section  ****
		if (DA == "TSA" ) { 
             // TSA is set up for use with LWMA1 only.  
             // It can't simulate ST with CN_delay because it depends on a per-block ST & D conncection.
                USE_CN_DELAY = 0;
             // CONSTANT_HR=1; //  This was for a different type of miner "attack" used in simulated_ST().
             TSA_Din = LWMA1_(TS,CD,T,N, height, FORK_HEIGHT, difficulty_guess);
      
             if (1) {
                // New method makes HR decision based on TSA_D.  Use CONSTANT_hr = 1;
                // Attacker checks TSA_D to decide if it's < his target
                simulated_ST = round((TSA_simulate_ST(T,R)*DX*TSA_Din)/(100*T)); 
                if ( TSA(TS,CD,T,N, height, FORK_HEIGHT, difficulty_guess,simulated_ST + TS.back(), R) < 
                          (attack_start*BASELINE_D)/100   )
                  {  HR = (baseline_HR*attack_size)/100; }
                else { HR = baseline_HR; }
                simulated_ST /= HR;
             }

             // old method.  Change CONSTANT_HR = 0 if you want to see motivations. Attacks have no effect.

             else { simulated_ST = round((TSA_simulate_ST(T,R)*DX*TSA_Din)/(100*T*HR)); }
             current_ST = simulated_ST; 
   		     uint64_t template_timestamp = current_ST + TS.back();
			 next_D =  TSA(TS,CD,T,N, height, FORK_HEIGHT, difficulty_guess, template_timestamp, R);  
		 } 
		//  **** End TSA section  ****

		// CD gets 1 block ahead of TS
	    CD.push_back(CD.back() + next_D);
	    if (CD.size() > N+1 ) { CD.erase(CD.begin()); }

		// Simulate solvetime this next_D
 
		if (DA != "TSA")  { 
	    simulated_ST = static_cast<u>(rand_values[i]*(CD.back()-CD[CD.size()-2])*DX/HR);
		}

		if (USE_CN_DELAY) {  current_ST = previous_ST; previous_ST = simulated_ST; }
		else { current_ST = simulated_ST; }  // TSA always
	// TS catches up with CD
	TS.push_back(TS.back() + current_ST); 
	if (TS.size() > N+1 ) { TS.erase(TS.begin()); }
	
	Ds[i]  = CD.back() - CD[CD.size()-2];
	if (DA == "TSA" ) { Ds[i] = TSA_Din; Douts[i] = CD.back() - CD[CD.size()-2]; } 
	HRs[i] = HR;	
	STs[i] = current_ST;
	if (i>2*N) {
		avgST += current_ST;
		avgD  += Ds[i];
		avgHR += HR;
	}
	if (PRINT_BLOCKS_TO_COMMAND_LINE) {  cout << i << "\t" << STs[i] << "\t" << Ds[i] << endl;   }
	
	// if (Ds[i] < 100 ) { cout << " D went < 100 at iteration " << i << " " << Ds[i] << endl; }
}
avgST /= (BLOCKS-2*N); avgHR /= (BLOCKS-2*N); avgD /= (BLOCKS-2*N); 
 
cout << DA << " " << N << " ST: " << avgST << " D: " << avgD << endl;

string temp = "blocks_" + DA + ".txt";	ofstream blocks_file(temp);

vector<float>nD(BLOCKS), nST(BLOCKS), nHR(BLOCKS), nST11(BLOCKS), nAttack(BLOCKS);

float SD(0),SD_ST(0);
int64_t j=0;
vector<int64_t>histotimes(600);
for (i=2*N+1;i<BLOCKS;i++) {	
	nD[i] = round(Ds[i]*100/avgD)/100;
	nHR[i] = round(HRs[i]*100/baseline_HR)/100;
	nST[i] = round(STs[i]*100/T)/100; 
	if (STs[i] < 601 ) { histotimes[STs[i]]++;} 
	if (DA == "TSA" ) { Douts[i] =round(Douts[i]*100/avgD)/100; }
	// if (nST[i] > 8 ) { cout << " ST > 8xT " << nST[i] << endl; }
 
	if (i >= 5 && i < BLOCKS-5) { 
		for (int j = i-5; j <= i+5; j++) { nST11[i] += STs[j]; };
		nAttack[i] = (11*T)/(nST11[i]+1);
		nST11[i]   = round((nST11[i]*100)/(T*11))/100; 
		nAttack[i] = round(nAttack[i]*100)/100;
		//		nST11[i] = accumulate(nST[i-5],nST[+5])/11; nAttack[i]=1/nST11[i]; 
	}  
	if (ENABLE_FILE_WRITES) { blocks_file << i+FORK_HEIGHT << "\t" << STs[i] << "\t" << Ds[i] << endl;} 

	SD += (Ds[i]-avgD)*(Ds[i]-avgD)/BLOCKS/avgD/avgD;
	SD_ST += (STs[i]-avgST)*(STs[i]-avgST)/BLOCKS/avgST/avgST;
}

 if (ENABLE_FILE_WRITES) { blocks_file.close(); }

  ofstream histo_file("histogram.txt");
  for (int i=0;i<599;i++) { histo_file << i << "\t" << histotimes[i] << endl; }
  histo_file.close();

if (ENABLE_FILE_WRITES) {

	temp = "plot_" + DA + to_string(IDENTIFIER) + ".txt";	ofstream plot_file(temp);
	for (i=2*N+1;i<BLOCKS;i++) {	
		plot_file << i+FORK_HEIGHT << "\t" << nD[i] << "\t" << nST[i] << "\t" << nHR[i] << "\t" << nST11[i] << "\t" << nAttack[i] << "\t" << Douts[i] << endl;
	}
	plot_file.close();
	int spacing = BLOCKS/60;
	int end = FORK_HEIGHT+BLOCKS;
 	temp = "gnuplot -c test_DAs_gnuplot.txt " + to_string(FORK_HEIGHT) + " " + to_string(end) + " " + 
			to_string(spacing) + " " + DA + to_string(IDENTIFIER) + " " + to_string(11) ;
	system((temp).c_str() );
	if ( DA == "DIGISHIELD_" ) { N=N-6; }
	html_file << "<B>" << DA << "</B> Target ST/avgST= " << T << "/" << avgST << " N= " << N;  
	if (attack_size != 100) { html_file << " attack_size: " << attack_size << " start/start: " 
	    << attack_start << "/" << attack_stop;}
	html_file << " StdDev Diffs: " << sqrt(SD) << " StdDev STs: " << sqrt(SD_ST) << 
 				"\n<br><img src=gif_" << DA << to_string(IDENTIFIER) << ".gif><br>" << endl;
}
}
 
int main() 
{
u N;
string DA;

srand(time(0)); // seed for fRand();


// These global constants are not typically changed for a given set of simulations.
// That's why they are global. 

BLOCKS = 10000; // number of BLOCKS to simulate
BASELINE_D = 1e5; // The average D just before the fork. 
if (BASELINE_D < 10) { 
		cout << "BASELINE_D needs to be > 10 because of they way HR is used" << endl; return 0;
}
FORK_HEIGHT = 0; // if 0, then pretend it is genesis.
START_TIMESTAMP = 1540000000; // exk = (exk*271828)/s; // 271828 must be same # digits as "s"0 is fine, or 1540000000 for typical times
START_CD = 1E9; // Beginning cumulative_difficulty. 0 is fine. 
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

//  ************** Set attack size ***************
u attack_start = 130; // 90 = 90% of baseline D.  
u attack_stop = 135; // 120 = 120% of baseline D.
//  NOTE: use attack size = 100 to turn off hash attacks.
u attack_size = 600; // HR multiple in percent of baseline HR. Set to 100 for no attacks

u difficulty_guess = BASELINE_D; 
u T = 100;
u baseline_HR = (BASELINE_D*DX)/T; // baseline_HR = (BASELINE_D*DX)/T; to match HR to baseline D
if (baseline_HR < 2) { 
	cout << "baseline_D/T ratio is too small to simulate with an integer HR." << endl; 
	exit(0); 
}
USE_CN_DELAY = 0; // 1 = yes, 0=no. CN coins have a delay on timestamps

// Do not plot if run is > 10,000
ENABLE_FILE_WRITES = 1;
PRINT_BLOCKS_TO_COMMAND_LINE = 1;
if (BLOCKS > 10000) { ENABLE_FILE_WRITES = 0; PRINT_BLOCKS_TO_COMMAND_LINE = 0;}
if (ENABLE_FILE_WRITES) {
	html_file << "<HTML><head><title>Difficulty Plots</title></head><body>" << endl;
}

// ********* Select Algos and Run **********

DA = "LWMA1_";
N = 144; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0); 

DA = "SMA_"; 
N = 144; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0); 



// *********    End program    *************

html_file.close();

exit(0); // return 0;



DA = "Boris_";
N = 500; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0);

// *********   the following is for copy-paste    ***************

DA = "EMA_";
N = 35; IDENTIFIER++;
uint64_t MTP = 11;// make sure negative solvetimes are not possible.
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0); 

DA = "DIGISHIELD_";
N = 17; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0);



DA="TSA";
// Unlike others, attack_start is relative to avg of N difficulties and is based on 
// TSA_D for a given timestamp
R = 1;  // only 1, 2, and 4 supported in simulation. This is only needed for TSA.
CONSTANT_HR =0;
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

 
DA = "DIGISHIELD_impoved_"; // problems removed
N = 17; IDENTIFIER++;
run_simulation(DA, T, N,  difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0); 


DA = "LWMA4_"; 
N = 60; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0); 

DA = "DIGISHIELD_";
N = 17; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0);
 
DA = "DIGISHIELD_impoved_"; // problems removed
N = 17; IDENTIFIER++;
run_simulation(DA, T, N,  difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0); 

DA = "DGW_"; // It's just a SMA_ with 1/3 and 3x limits on ST
N=24; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0); 


// Repeat, but now do it for constant HR.
attack_size=100;

DA = "SMA_"; 
N = 45; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0); 

DA = "DGW_"; // It's just a SMA_ with 1/3 and 3x limits on ST
N=24; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0); 


} ;


vector<u>TSA_t_linear_HR_N_4{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,203,204,206,207,209,210,212,214,216,218,219,222,225,229,234,240,247,256};

vector<u>TSA_t_constant_HR_N_2{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,208,209,210,212,213,215,217,219,221,223,226,229,231,236,240,246,255,269};
vector<u>TSA_x_constant_HR_N_2{994,989,984,979,974,969,963,958,953,947,942,936,930,925,919,913,908,902,896,890,884,878,872,866,860,854,848,842,835,829,823,817,810,804,797,791,785,778,772,765,758,752,745,739,732,725,719,712,705,699,692,685,678,672,665,658,651,644,638,631,624,617,610,603,597,590,583,576,569,563,556,549,542,536,529,522,515,509,502,495,489,482,475,469,462,456,449,443,436,430,423,417,411,404,398,392,386,380,373,367,361,355,349,343,338,332,326,320,314,309,303,298,292,287,281,276,271,265,260,255,250,245,240,235,230,225,221,216,211,207,202,198,193,189,184,180,176,172,168,164,160,156,152,148,145,141,138,134,131,127,124,121,117,114,111,108,105,102,99,96,94,91,88,86,83,80,78,76,73,71,69,67,64,62,60,58,56,55,53,51,49,47,46,44,43,41,40,38,37,35,34,33,32,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0};
vector<u>TSA_x_linear_HR_N_2{997,994,992,989,986,983,980,977,974,970,967,963,960,956,952,949,945,941,937,932,928,924,919,915,910,905,900,895,890,885,880,875,869,864,858,853,847,841,835,829,823,817,810,804,798,791,784,778,771,764,757,750,743,736,729,721,714,707,699,692,684,676,669,661,653,645,637,629,621,613,605,597,589,581,573,564,556,548,540,531,523,515,506,498,490,481,473,465,457,448,440,432,424,416,407,399,391,383,375,367,359,352,344,336,329,321,313,306,299,291,284,277,270,263,256,249,242,235,229,222,216,210,204,197,191,186,180,174,168,163,158,152,147,142,137,132,128,123,118,114,110,106,101,97,94,90,86,83,79,76,73,69,66,63,60,58,55,52,50,48,45,43,41,39,37,35,33,31,30,28,26,25,23,22,21,20,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0};
vector<u>TSA_t_linear_HR_N_2{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,185,186,187,189,191,193,195,198,200,204,210,222};
vector<u>TSA_x, TSA_t;
   if (R == 2 && CONSTANT_HR) { TSA_x = TSA_x_constant_HR_N_2; TSA_t = TSA_t_constant_HR_N_2;}
   else if (R == 2 && !CONSTANT_HR) { TSA_x = TSA_x_linear_HR_N_2; TSA_t = TSA_t_linear_HR_N_2;}
   else if (R == 4 && CONSTANT_HR) { TSA_x = TSA_x_constant_HR_N_4; TSA_t = TSA_t_constant_HR_N_4;}
   else if (R == 4 && !CONSTANT_HR) { TSA_x = TSA_x_linear_HR_N_4; TSA_t = TSA_t_linear_HR_N_4;}
   else { cout << "Only M=2 and M=4 are supported in TSA simulation." << endl; }

   for (int j=0; j < TSA_x.size(); j++ ) {  
      if ( x >= TSA_x[j] ) { return (TSA_t[j]*T)/100;  }
   }
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

// ========   TSA   ===========

difficulty_type TSA(std::vector<uint64_t> timestamps, 
      std::vector<uint64_t> cumulative_difficulties, uint64_t T, uint64_t N, uint64_t height,  
            uint64_t FORK_HEIGHT, uint64_t  difficulty_guess, uint64_t networkTime, int64_t R ) {
   uint64_t  L(0), next_D, i, this_timestamp(0), previous_timestamp(0), avg_D;

   assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );
   // Hard code D if there are not at least N+1 BLOCKS after fork or genesis
   if (height >= FORK_HEIGHT && height < FORK_HEIGHT + N) { return difficulty_guess; }
   assert(timestamps.size() == N+1); 
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
   if (avg_D > 2000000*N*N*T) { next_D = (avg_D/(200*L))*(N*(N+1)*T*99);  }   
   else {    next_D = (avg_D*N*(N+1)*T*99)/(200*L);    }	

// LWMA is finished, now use its next_D and previous_timestamp 
// to get TSA's next_D.  I had to shift from unsigned to singed integers.
  
   int64_t TSA_D = next_D, Ta = T, TR = Ta*R, m = 1E3;
   int64_t exm = m; // this will become e^x * 1E6.
   int64_t ST =  static_cast<int64_t>(networkTime) - static_cast<int64_t>(previous_timestamp);
   ST = std::max(1+Ta/100,std::min(ST,10*Ta));
   for ( int i = 1; i <= ST/TR ; i++ ) { exm = (exm*static_cast<int64_t>(2.718*m))/m; } 
   int64_t f = ST % (TR); 
   exm = (exm*(m+(f*(m+(f*(m+(f*(m+(f*m)/(4*TR)))/(3*TR)))/(2*TR)))/(TR)))/m;
   TSA_D = (TSA_D*(1000*(m*ST))/(m*Ta+(ST-Ta)*exm))/1000;
   // Make all insignificant digits zero for easy reading.
   i = 1000000000;
   while (i > 1) { 
      if ( TSA_D > i*100 ) { TSA_D = ((TSA_D+i/2)/i)*i; break; }
      else { i /= 10; }
   }
   if (R == 2) { TSA_D = (TSA_D*955)/1000; } // adjustment based on experiment
   return static_cast<uint64_t>(TSA_D);  	
}


// =======   Simple Moving average  ========

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
// ========== Dark Gravity Wave  ===========

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

// ========    Digishield    ===========

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

// =========   Digishield without MTP=11 delay and no limit on ST  ===============

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

//  ==========  EMA_ difficulty algorithm    ============
difficulty_type EMA_(std::vector<uint64_t> timestamps, 
   std::vector<uint64_t> cumulative_difficulties, uint64_t uT, uint64_t uN, uint64_t height,
					uint64_t FORK_HEIGHT,uint64_t  difficulty_guess) {
  // The last 3 input variables above are not used. They're their for consistency with the other DAs.
  // assert(N>2); 
	assert(difficulty_guess > 99); // D must be > 99
	
	int64_t T=uT,N=uN;

	// Safely prevent out of sequence timestamps.
	int64_t previous_timestamp(0), this_timestamp(0), ST, i;
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
	int64_t s = 1E6;
	int64_t exk = s; 
	for ( int i = 1; i <= ST/T/N ; i++ ) { 	exk = (exk*static_cast<int64_t>(2.71828*s))/s; 	} 
	int64_t tm = ST % (T*N); 
	exk = (exk*(s+(tm*(s+(tm*(s+(tm*(s+(tm*s)/(4*T*N)))/(3*T*N)))/(2*T*N)))/(T*N)))/s;
	int64_t prev_D = static_cast<uint64_t>(cumulative_difficulties.back() - cumulative_difficulties[cumulative_difficulties.size()-2]);
   // 10000 helps prevent overflow, but causes error for M > 15. 
	 int64_t next_D = std::max(static_cast<int64_t>(10), (prev_D*(s*ST))/(s*T+(ST-T)*exk)); 
   if ( N==2) { next_D = (next_D*92)/100; }
   else if ( N==3) { next_D = (next_D*98)/100; }
   else if ( N==4) { next_D = (next_D*99)/100; }
  // int64_t next_D = std::max(static_cast<int64_t>(10),(prev_D*(100000*T+(1000*s*(ST-T))/exk)/ST)/100000); 
  // Target Methods are just inverse of the Difficulty method (do parenthesis carefully)
  // arith_uint256  TSATarget(0);
  // TSATarget = (nextTarget*(m*T+(ST-T)*exm))/(m*ST);
	// For non-small N, this works as well as all the exk mess.
 //  int64_t next_D = std::max(static_cast<int64_t>(10),(prev_D*N*T)/(T*N+ST-T)); // TH method who's denominator can't go negative.
	return static_cast<uint64_t>(next_D);  	
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
   if (height >= FORK_HEIGHT && height <= FORK_HEIGHT + N+1) { return difficulty_guess; }
   assert(timestamps.size() == N+1); 
   

   uint64_t  L(0), next_D, i, this_timestamp(0), previous_timestamp(0), avg_D;

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
    
   // Genesis should be the only time sizes are < N+1.
   assert(timestamps.size() == cumulative_difficulties.size() && timestamps.size() <= N+1 );

  // Hard code D if there are not at least N+1 BLOCKS after fork (or genesis)
   if (height >= FORK_HEIGHT && height <= FORK_HEIGHT + N+1) { return difficulty_guess; }
   assert(timestamps.size() == N+1); 

   uint64_t  L(0), ST(0), next_D, prev_D, avg_D, i;
        

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

int run_simulation(string DA, u T, u N,u difficulty_guess,u baseline_HR,u attack_start,u attack_stop,u attack_size, int64_t R) {

if ( DA == "DIGISHIELD_" ) { N=N+6; } // For simulated MTP = 11 delay.

	// In theory, this just uses ST = -ln(rand())* T * D/HR, but it's a mess, half-way because of CN_delay.
  // I should have made two different methods.  One with CN delay and one without.
  // Even without that it would have been a mess because of genesis or fork option.  Needed 4 methods.

	u next_D(0), TSA_Din, simulated_ST(0), i, HR(0), attack_on(0);
	float avgST(0), avgHR(0), avgD(0);
	u previous_ST, current_ST;
	vector<u>STs(BLOCKS);
  vector<u>Ds(BLOCKS);
	vector<u>HRs(BLOCKS);
  vector<float>Douts(BLOCKS,0); // TSA will be the only 1 to change the all-0 values

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
		cout << "You can't fork if genesis was < N+1 BLOCKS in the past." << endl; 
	}
// *****  Run simulation  ********
previous_ST = T; // initialize in case we are using CN delay.
HR = baseline_HR;
u height = FORK_HEIGHT; // This is kind of dummy work to send DA's what they need.

for (i=0; i <= BLOCKS-1; i++) {
	// attack_size = 0 means there's no hash attack, but just constant HR.
	if (DA == "TSA" ) {
      next_D = LWMA1_(TS,CD,T,N, height, FORK_HEIGHT, difficulty_guess);
	}
	if (next_D > (attack_stop*BASELINE_D)/100  ) { 
		attack_on = 0;
		HR = baseline_HR;
	}
	else if (next_D < (attack_start*BASELINE_D)/100 ) {
		attack_on = 1;
		HR = (baseline_HR*attack_size)/100;
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
		if (DA == "TSA" ) { 
      // TSA is set up for use with LWMA1 only.  
      // It can't simulate ST with CN_delay because it depends on a per-block ST & D conncection.
      USE_CN_DELAY = 0;
      next_D = LWMA1_(TS,CD,T,N, height, FORK_HEIGHT, difficulty_guess);
			TSA_Din = next_D;
      simulated_ST = (TSA_simulate_ST(T,R)*DX*TSA_Din)/(100*HR); // 100 is b/c it's returned as percent
   
			if (USE_CN_DELAY) {  current_ST = previous_ST; previous_ST = simulated_ST; }
      else { current_ST = simulated_ST; }
			// simulated_ST = static_cast<u>(log( 1/fRand(0,1))*static_cast<float>((CD.back()-CD[CD.size()-2])*DX)/HR);
   		uint64_t nTime = current_ST + TS.back();
			next_D =  TSA(TS,CD,T,N, height, FORK_HEIGHT, difficulty_guess, nTime, R);  
		}

		// CD gets 1 block ahead of TS
	  CD.push_back(CD.back() + next_D);
	  if (CD.size() > N+1 ) { CD.erase(CD.begin()); }
	}
  // Simulate soolvetime this next_D
 
  if (DA != "TSA" ) { 
	    simulated_ST = static_cast<u>(log( 1/fRand(0,1))*static_cast<float>((CD.back()-CD[CD.size()-2])*DX)/HR);
  }

	if (USE_CN_DELAY) {  current_ST = previous_ST; previous_ST = simulated_ST; }
  else { current_ST = simulated_ST; }
	// TS catches up with CD
	TS.push_back(TS.back() + current_ST); 
	if (TS.size() > N+1 ) { TS.erase(TS.begin()); }
	
	Ds[i]  = CD.back() - CD[CD.size()-2];
	if (DA == "TSA" ) { Ds[i] = TSA_Din; Douts[i] = CD.back() - CD[CD.size()-2]; } 
	HRs[i] = HR;	
	STs[i] = current_ST;
  avgST += current_ST;
	avgD  += Ds[i];
  avgHR += HR;
	if (PRINT_BLOCKS_TO_COMMAND_LINE) { 
    cout << i << "\t" << STs[i] << "\t" << Ds[i] << "\t" << TS[i] << "\t" << CD[i] << endl; 
  }
	
	// if (Ds[i] < 100 ) { cout << " D went < 100 at iteration " << i << " " << Ds[i] << endl; }
}
avgST /= BLOCKS; avgHR /= BLOCKS; avgD /= BLOCKS; 
 
	if (PRINT_BLOCKS_TO_COMMAND_LINE) {  cout << "height, ST, D, Timestamp, CD" << endl; }
cout << DA << ": avgST: " << avgST << " avgD: " << avgD << endl;

string temp = "blocks_" + DA + ".txt";	ofstream file(temp);


vector<float>nD(BLOCKS), nST(BLOCKS), nHR(BLOCKS), nST11(BLOCKS), nAttack(BLOCKS);

float SD(0);
for (i=0;i<BLOCKS;i++) {	
	nD[i] = round(Ds[i]*100/avgD)/100;
	nHR[i] = round(HRs[i]*100/baseline_HR)/100;
	nST[i] = round(STs[i]*100/T)/100;
  if (DA == "TSA" ) { Douts[i] =round(Douts[i]*100/avgD)/100; }
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
		file2 << i+FORK_HEIGHT << "\t" << nD[i] << "\t" << nST[i] << "\t" << nHR[i] << "\t" << nST11[i] << "\t" << nAttack[i] << "\t" << Douts[i] << endl;
	}
	file2.close();
	int spacing = BLOCKS/60;
	int end = FORK_HEIGHT+BLOCKS;
 	temp = "gnuplot -c test_DAs_gnuplot.txt " + to_string(FORK_HEIGHT) + " " + to_string(end) + " " + 
			to_string(spacing) + " " + DA + to_string(IDENTIFIER) + " " + to_string(11) ;
	system((temp).c_str() );
	if ( DA == "DIGISHIELD_" ) { N=N-6; }
	html_file << "<B>" << DA << "</B> Target ST/avgST= " << T << "/" << avgST << " N= " << N << " attack_size: " << 
         attack_size << " start/start: " << attack_start << "/" << attack_stop << " StdDev: " << sqrt(SD) <<
 				"\n<br><img src=gif_" << DA << to_string(IDENTIFIER) << ".gif><br>" << endl;
}
}
 
int main() 
{

srand(time(0)); // seed for fRand();

// These global constants are not typically changed for a given set of simulations.
// That's why they are global. 

BLOCKS = 1000; // number of BLOCKS to simulate
BASELINE_D = 40001; // The average D just before the fork. 
if (BASELINE_D < 10) { 
		cout << "BASELINE_D needs to be > 10 because of they way HR is used" << endl; return 0;
}
FORK_HEIGHT = 0; // if 0, then pretend it is genesis.
START_TIMESTAMP = 1540000000; // exk = (exk*271828)/s;      // 271828 must be same # digits as "s"0 is fine, or 1540000000 for typical times
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

u attack_start = 120; // 90 = 90% of baseline D.  
u attack_stop = 160; // 120 = 120% of baseline D.
//  NOTE: use attack size = 100 to turn off hash attacks.
u attack_size = 300; // HR multiple of baseline HR. Set to 100 for no attacks
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
PRINT_BLOCKS_TO_COMMAND_LINE = 0;

DA = "TSA";  // This is only set up to use LWMA1 with N=4
// TSA has setting inside the loop to choose the per block constant_HR or linear motivation HR.
// This is not related to hash attacks which are based on seeing Din.
N = 60; IDENTIFIER++;
// The following is special for TSA only. Miner motivation to not mine until D is lower during block.
CONSTANT_HR=1; // If "1" ST simulates dedicated miners. "O" indicates a lineaar motivation 
R = 4;  // only 2 and 4 supported in simulation. This is only needed for TSA.
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size,R); 

DA = "LWMA1_";
N = 60; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0); 

DA = "LWMA4_"; 
N = 60; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0); 

DA = "DIGISHIELD_";
N = 17; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0);
 
DA = "DIGISHIELD_impoved_"; // problems removed
N = 17; IDENTIFIER++;
run_simulation(DA, T, N,  difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0); 

DA = "EMA_";
N = 40; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0); 
 
DA = "SMA_"; 
N = 45; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0); 

DA = "DGW_"; // It's just a SMA_ with 1/3 and 3x limits on ST
N=24; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0); 


// Repeat, but now do it for constant HR.
attack_size=100;



DA = "TSA";  // This is only set up to use LWMA1 with N=4
// TSA has setting inside the loop to choose the per block constant_HR or linear motivation HR.
// This is not related to hash attacks which are based on seeing Din.
N = 60; IDENTIFIER++;
// The following is special for TSA only. Miner motivation to not mine until D is lower during block.
CONSTANT_HR=1; // If "1" ST simulates dedicated miners. "O" indicates a lineaar motivation 
R = 4;  // only 2 and 4 supported in simulation. This is only needed for TSA.
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size,R); 

DA = "LWMA1_";
N = 60; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0); 

DA = "LWMA4_"; 
N = 60; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0); 

DA = "DIGISHIELD_";
N = 17; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0);
 
DA = "DIGISHIELD_impoved_"; // problems removed
N = 17; IDENTIFIER++;
run_simulation(DA, T, N,  difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0); 

DA = "EMA_";
N = 40; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0); 
 
DA = "SMA_"; 
N = 45; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0); 

DA = "DGW_"; // It's just a SMA_ with 1/3 and 3x limits on ST
N=24; IDENTIFIER++;
run_simulation(DA, T, N, difficulty_guess, baseline_HR, attack_start,attack_stop,attack_size, 0); 


html_file.close();
} ;

