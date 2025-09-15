#pragma once
#include <vector>
#include <string>

using namespace std;

#ifndef OPTIMIZATION_H_
#define OPTIMIZATION_H_

class Optimization {

public:

	Optimization();
	virtual ~Optimization();

	void Algorithm(); //最適化アルゴリズム

private:
	void GameBase(); //ピクロス盤面の確定マスの選定
	void Initialization(int loop); //初期化
	void Crossover(int loop); //交叉
	void Mutation(int gene, int loop); //突然変異
	int Selection_Elite(int gene); //選択
	int Selection_tonament(int gene);
	int selection_roulette();
	int selection_nsga2(); //NAGA2による選択
	void Plot_result(int i, int simu, string filename); //検証後の評価値プロット
	void Adjust_blackcell(int i, int r1, int r2, int c1, int c2,int loop);
	void EvaluationFunction(int i); //評価関数の算出

	vector <int> column_copy; //列の複製
	
	long NUM_CELL; //必要配置黒マス数

	struct INDIVIDUAL { //解の構造体
		vector<vector<int> > BOARD; //ゲーム盤面
		vector<long> EVALUATION_VALUE_LINE; //行の評価値
		vector<long> EVALUATION_VALUE_COLUMN; //列の評価値

		double EVALUATION_VALUE_F_1; //列の評価値の合計（目的関数１）
		double EVALUATION_VALUE_F_2; //列の評価値の合計（目的関数２）

	};
	vector<INDIVIDUAL> IDV;

	struct TMP_INDIVIDUAL { //解の構造体
		vector<vector<int> > BOARD; //ゲーム盤面
		vector<long> EVALUATION_VALUE_LINE; //行の評価値
		vector<long> EVALUATION_VALUE_COLUMN; //列の評価値

		double EVALUATION_VALUE_F_1; //列の評価値の合計（目的関数１）
		double EVALUATION_VALUE_F_2; //列の評価値の合計（目的関数２）
	};
	vector<INDIVIDUAL> TMP_IDV;

	vector<INDIVIDUAL> SelectElites; //エリート選択

};

#endif

