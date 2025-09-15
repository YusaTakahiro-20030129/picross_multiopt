#include "Parameters.h"

const double PROBABILITY_LOWER_LIMIT_LINE = 1.0; //確定マスとする確率

const int SIMULATION_SIZE = 20; //シミュレーション回数
const int GENERATION_SIZE = 400; //GAの世代数
const int PARENTS_SIZE = 500; //GAの親集団サイズ
const int OFFSPRING_SIZE = PARENTS_SIZE * 3; //GAの子集団サイズ

const int TOURNAMENT_SIZE = 2 * (PARENTS_SIZE + OFFSPRING_SIZE) / PARENTS_SIZE; //GAの選択のトーナメントサイズ

// 確定マスの設定
// 0:確定マスを設定しない , 1：確定マスを設定する
const int FIXCELL_FLAG = 1;

// 交叉方法の設定
// 0: 領域交叉, 1: 2行交叉, 2: 半数数行交叉・半数列交叉
const int CROSSOVER_FLAG = 2;

// 交叉時の相手の親選択方法
// 0: ランダム選択, 1: 異サブ集団選択, 2: トーナメント選択
const int PARENTS_FLAG = 1;

// 突然変異方法の設定
// 0: 領域変異（完全ランダム）, 1: 領域変異（確定マス設定）, 2: 半数行変異・半数列変異(確定マス設定)
const int MUTATION_FLAG = 1;

// 選択方法の設定
// 0: エリート選択, 1: トーナメント選択
const int SELECTION_FLAG = 1;

GAME_BOARD GB; //ゲーム盤面の構造体kk
vector<vector<BEST_INDIVIDUAL> > BT_IDV; //各シミュレーションの最良解の構造体

const size_t NUM_LINE = GB.HINTS_LINE.size(); //行数
const size_t NUM_COL = GB.HINTS_COLUMN.size(); //列数