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
	vector<int> fixcell_flags = {0, 1};
	vector<int> crossover_flags = {0, 1, 2};
	vector<int> parents_flags = {0, 1, 2};
	vector<int> mutation_flags = {0, 1, 2};
	vector<int> selection_flags = {0, 1};

	int total_run = 0;


	// // 全てのパラメータ設定の組み合わせで実行
	// for (int fix : fixcell_flags)
	// 	for (int cross : crossover_flags)
	// 		for (int parent : parents_flags)
	// 			for (int mut : mutation_flags)
	// 				for (int sel : selection_flags)
	// 				{
	// 					// Parameters.h のグローバル変数を設定
	// 					FIXCELL_FLAG = fix;
	// 					CROSSOVER_FLAG = cross;
	// 					PARENTS_FLAG = parent;
	// 					MUTATION_FLAG = mut;
	// 					SELECTION_FLAG = sel;

	// 					cout << "=== simu START ===" << endl;
	// 					cout << "FIXCELL:" << fix
	// 						 << ", CROSSOVER:" << cross
	// 						 << ", PARENTS:" << parent
	// 						 << ", MUTATION:" << mut
	// 						 << ", SELECTION:" << sel << endl;

						// 各パラメータ設定で最適化実行
						Optimization opt;
						opt.Algorithm();

					// 	total_run++;
					// }

	cout << "All Completed" << endl;
	cout <<  "opt_time : " << OPT_TIME << endl;
	return 0;
}