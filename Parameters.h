#include <vector>
#include "Question.h"

using namespace std;

#ifndef PARAMETERS_H_
#define PARAMETERS_H_

// 現在使用する問題の切り替え（ここを変えるだけ！）
#define CURRENT_LINE_HINTS Hints_Line_gun
#define CURRENT_COLUMN_HINTS Hints_Col_gun

extern const double PROBABILITY_LOWER_LIMIT_LINE; // 確率的実行可能行の下限値
extern int OPT_TIME;                              // 最適解到達回数
extern const size_t NUM_LINE;                     // 行数
extern const size_t NUM_COL;                      // 列数
extern const int SIMULATION_SIZE;                 // シミュレーション回数
extern const int GENERATION_SIZE;                 // GAの世代数
extern const int PARENTS_SIZE;                    // GAの親集団サイズ
extern const int OFFSPRING_SIZE;                  // GAの子集団サイズ
extern const double MUTATION_RATE;                // GAの突然変異確率

extern const int TOURNAMENT_SIZE; // GAの選択のトーナメントサイズ
extern const int TOURNAMENT_RATE; // 交叉のトーナメント選択の選択割合
extern const int MEMETIC_GENE; // 非改善トリガー局所探索の非改善許容世代数
extern const int MEMETIC_RATE; // 確率的局所探索の確率

// 実験時の分岐のためのフラグ設定（数値に応じてやりたい動作ができる）
extern int INITIALIZATION_FLAG; // 初期集団の生成方法
extern int FIXCELL_FLAG;   // 確定マスの設定
extern int CROSSOVER_FLAG; // 交叉方法の設定
extern int PARENTS_FLAG;   // 交叉時の親選択方法
extern int MUTATION_FLAG;  // 突然変異方法の設定
extern int MEMETIC_FLAG;   // 局所探索の設定
extern int SELECTION_FLAG; // 次世代集団生成方法の設定

// 乱数の再現性の担保のためのシード値
constexpr int BASE_SEED = 12345;
constexpr int LOOP_OFFSET = 1000000;
constexpr int OPERATION_OFFSET = 10000;
constexpr int INDIVIDUAL_OFFSET = 1;
constexpr int OP_ADJUST_BLACKCELL = 100;

extern int FINISH_GENERATION; // 終了世代の格納(最適解に到達した時はその世代、未到達なら最終世代)

struct GAME_BOARD
{ // ゲーム盤面の構造体

    vector<vector<int>> BOARD; // 盤面

    vector<vector<int>> BOARD_FIX_CELL;
    vector<vector<int>> BOARD_FIX_CELL_TMP;

    // 行ヒント
    vector<vector<int>> HINTS_LINE = CURRENT_LINE_HINTS;

    // 列ヒント
    vector<vector<int>> HINTS_COLUMN = CURRENT_COLUMN_HINTS;

    vector<int> BLACKCELLFIXNUM_LINE;   // 行ごとの黒マスの確定マス数
    vector<int> BLACKCELLFIXNUM_COLUMN;  // 列ごとの黒マスの確定マス数

    vector<vector<vector<int>>> FEASIBLE_LINE;             // 実行可能行
    vector<vector<vector<int>>> PROBABILITY_FEASIBLE_LINE; // 確率的実行可能行
    vector<vector<vector<int>>> FEASIBLE_COLUMN;           // 実行可能列

    vector<int> BLACKCELLNUM_LINE;   // 行ごとの黒マスの個数
    vector<int> BLACKCELLNUM_COLUMN;  // 列ごとの黒マスの個数

    vector<vector<double>> BLACK_PROBABILITY_LINE;   // 行の黒マスの確率
    vector<vector<double>> BLACK_PROBABILITY_COLUMN; // 列の黒マスの確率
};
extern GAME_BOARD GB;

struct BEST_INDIVIDUAL
{                                         // 各シミュレーションの最良解の構造体
    vector<vector<int>> BOARD;            // ゲーム盤面
    vector<long> EVALUATION_VALUE_LINE;   // 行の評価値
    vector<long> EVALUATION_VALUE_COLUMN; // 列の評価値

    double EVALUATION_VALUE_F_1; // 列の評価値の合計（目的関数１）
    double EVALUATION_VALUE_F_2; // 列の評価値の合計（目的関数２）
};
extern vector<vector<BEST_INDIVIDUAL>> BT_IDV;

#endif
