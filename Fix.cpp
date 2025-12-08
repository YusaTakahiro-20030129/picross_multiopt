#include "Parameters.h"
#include "Optimization.h"
#include "Parameters.cpp"
#include "Optimization.cpp"
#include "SeedUtil.h"
#include "Question.h"

#include <iostream>
#include <climits>

using namespace std;

int main()
{

	Optimization opt;
	opt.Algorithm();

	cout << "All Completed" << endl;
	cout << "opt_time : " << OPT_TIME << endl;
	return 0;
}