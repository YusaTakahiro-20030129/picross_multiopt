#include "Parameters.h"
#include "Optimization.h"
#include "SeedUtil.h"

#include <iostream>
#include <iomanip>
#include <random>
#include <climits>
#include <string>
#include <fstream>
#include <ctime>
#include <algorithm>
#include <set>
#include <numeric>
#include <limits>
#include <sys/stat.h>
#include <sys/types.h>
#include <filesystem>

using namespace std;

static mt19937_64 mt;

// コンストラクタ
Optimization ::Optimization()
{

    // 使用する各要素を初期化する
    GB.BOARD.resize(NUM_LINE); // ゲーム盤面の構造体の初期化
    GB.BOARD_FIX_CELL.resize(NUM_LINE);
    GB.BOARD_FIX_CELL_TMP.resize(NUM_LINE);
    GB.BLACK_PROBABILITY_LINE.resize(NUM_LINE);
    GB.BLACK_PROBABILITY_COLUMN.resize(NUM_LINE);
    GB.BLACKCELLNUM_LINE.resize(NUM_LINE);
    GB.BLACKCELLNUM_COLUMN.resize(NUM_COL);
    GB.BLACKCELLFIXNUM_LINE.resize(NUM_LINE);
    GB.BLACKCELLFIXNUM_COLUMN.resize(NUM_COL);

    for (int i = 0; i < NUM_LINE; i++)
    {
        GB.BOARD[i].resize(NUM_COL);
        GB.BOARD_FIX_CELL[i].resize(NUM_COL);
        GB.BOARD_FIX_CELL_TMP[i].resize(NUM_COL);
        GB.BLACK_PROBABILITY_LINE[i].resize(NUM_COL);
        GB.BLACK_PROBABILITY_COLUMN[i].resize(NUM_COL);
    }

    GB.FEASIBLE_LINE.resize(NUM_LINE);
    GB.PROBABILITY_FEASIBLE_LINE.resize(NUM_LINE);
    GB.FEASIBLE_COLUMN.resize(NUM_COL);

    BT_IDV.resize(SIMULATION_SIZE); // 各シミュレーションの最良解の構造体の初期化

    for (int simu = 0; simu < SIMULATION_SIZE; simu++)
    {
        BT_IDV[simu].resize(GENERATION_SIZE);
        for (int gene = 0; gene < GENERATION_SIZE; gene++)
        {
            BT_IDV[simu][gene].BOARD.resize(NUM_LINE);
            for (int i = 0; i < NUM_LINE; i++)
            {
                BT_IDV[simu][gene].BOARD[i].resize(NUM_COL);
            }
            BT_IDV[simu][gene].EVALUATION_VALUE_LINE.resize(NUM_LINE);
            BT_IDV[simu][gene].EVALUATION_VALUE_COLUMN.resize(NUM_COL);
        }
    }
    vector<int> tmp_next(PARENTS_SIZE);
}

// デコンストラクタ
Optimization ::~Optimization()
{
}

void Optimization::Algorithm()
{

    GameBase(); // ピクロス盤面の確定マスの選定
    long feasible_line_num = 0, probability_feasible_line_num = 0;
    // cout << "all feasible line,  probabilistic feasible line," << endl;
    for (int i = 0; i < NUM_LINE; i++)
    {
        // cout << GB.FEASIBLE_LINE[i].size()<<","<<GB.PROBABILITY_FEASIBLE_LINE[i].size()<<endl;
        feasible_line_num += GB.FEASIBLE_LINE[i].size();
        probability_feasible_line_num += GB.PROBABILITY_FEASIBLE_LINE[i].size();
    }
    cout << "OK" << endl;

    // 最適解到達回数カウンター
    OPT_TIME = 0;

    for (int simu = 0; simu < SIMULATION_SIZE; simu++)
    { // GAによる最適化
        // GA停滞検出（simuごとにリセット）
        int ga_stagnation_count = 0;
        double ga_prev_best_eval = numeric_limits<double>::max();
        for (int gene = 0; gene < GENERATION_SIZE; gene++)
        {
            if (gene == 0)
            {
                int seed_init = generate_seed(simu, 0, 0);
                Initialization(seed_init); // 初期化
            }
            else
            {
                int seed_cross = generate_seed(simu, 1, 0);
                Crossover(seed_cross); // 交叉

                int seed_mut = generate_seed(simu, 2, 0);
                Mutation(gene, seed_mut); // 突然変異
            }
            int BestIndividualNo = 0;
            // 選択(次世代集団生成)
            if (SELECTION_FLAG == 0)
            {
                // エリート選択
                BestIndividualNo = Selection_Elite(gene);
            }
            if (SELECTION_FLAG == 1)
            {
                // トーナメント選択
                BestIndividualNo = Selection_tonament(gene);
            }

            // Selection後の GA best を取得（停滞判定用）
            int GA_BestNo = 0;
            double GA_best_eval = numeric_limits<double>::max();
            for (int i = 0; i < PARENTS_SIZE; i++)
            {
                double f1 = IDV[i].EVALUATION_VALUE_F_1;
                double f2 = IDV[i].EVALUATION_VALUE_F_2;
                double val = f1 * f1 + f2 * f2;
                if (val < GA_best_eval)
                {
                    GA_best_eval = val;
                    GA_BestNo = i;
                }
            }

            // 停滞カウント更新（GA best 基準）
            if (GA_best_eval < ga_prev_best_eval)
            {
                ga_prev_best_eval = GA_best_eval;
                ga_stagnation_count = 0;
            }
            else
            {
                ga_stagnation_count++;
            }

            // 5世代連続停滞なら LS 注入（GA best 基準）
            if (GA_FLAG == 1 && ga_stagnation_count >= LS_STAGNATION_GENE)
            {
                long before_f1 = IDV[GA_BestNo].EVALUATION_VALUE_F_1;
                long before_f2 = IDV[GA_BestNo].EVALUATION_VALUE_F_2;

                LocalSearch(GA_BestNo); // ★ 01ビット反転LS（first-improvement）

                long after_f1 = IDV[GA_BestNo].EVALUATION_VALUE_F_1;
                long after_f2 = IDV[GA_BestNo].EVALUATION_VALUE_F_2;

                cout << "[LS injected] simu=" << simu << " gene=" << gene
                     << " stagnation=" << ga_stagnation_count
                     << " (" << before_f1 << "," << before_f2 << " -> "
                     << after_f1 << "," << after_f2 << ")" << endl;

                // LSを入れたので停滞カウントをリセット（次の停滞検出のため）
                ga_prev_best_eval = after_f1 * after_f1 + after_f2 * after_f2;
                ga_stagnation_count = 0;
            }

            // 世代終了時点の最終 best を再取得して BT_IDV に保存
            int FinalBestNo = 0;
            double best_val = numeric_limits<double>::max();
            for (int i = 0; i < PARENTS_SIZE; i++)
            {
                double f1 = IDV[i].EVALUATION_VALUE_F_1;
                double f2 = IDV[i].EVALUATION_VALUE_F_2;
                double val = f1 * f1 + f2 * f2;
                if (val < best_val)
                {
                    best_val = val;
                    FinalBestNo = i;
                }
            }

            BT_IDV[simu][gene].EVALUATION_VALUE_F_1 = IDV[FinalBestNo].EVALUATION_VALUE_F_1;
            BT_IDV[simu][gene].EVALUATION_VALUE_F_2 = IDV[FinalBestNo].EVALUATION_VALUE_F_2;
            BT_IDV[simu][gene].EVALUATION_VALUE_COLUMN = IDV[FinalBestNo].EVALUATION_VALUE_COLUMN;
            BT_IDV[simu][gene].EVALUATION_VALUE_LINE = IDV[FinalBestNo].EVALUATION_VALUE_LINE;
            BT_IDV[simu][gene].BOARD = IDV[FinalBestNo].BOARD;

            cout << "simu : " << simu << " || gene " << gene
                 << ": best=(" << BT_IDV[simu][gene].EVALUATION_VALUE_F_1
                 << "," << BT_IDV[simu][gene].EVALUATION_VALUE_F_2 << ")" << endl;

            Plot_result(simu, gene, "pareto_");
            BestScore_Result(simu, gene);
            if (BT_IDV[simu][gene].EVALUATION_VALUE_F_1 == 0 && BT_IDV[simu][gene].EVALUATION_VALUE_F_2 == 0)
            { // 最適解に到達したら
                for (int k = gene + 1; k < GENERATION_SIZE; k++)
                { // 残り世代に最適解を保存
                    BT_IDV[simu][k].EVALUATION_VALUE_F_1 = BT_IDV[simu][gene].EVALUATION_VALUE_F_1;
                    BT_IDV[simu][k].EVALUATION_VALUE_F_2 = BT_IDV[simu][gene].EVALUATION_VALUE_F_2;
                    BT_IDV[simu][k].EVALUATION_VALUE_COLUMN = BT_IDV[simu][gene].EVALUATION_VALUE_COLUMN;
                    BT_IDV[simu][k].EVALUATION_VALUE_LINE = BT_IDV[simu][gene].EVALUATION_VALUE_LINE;
                    BT_IDV[simu][k].BOARD = BT_IDV[simu][gene].BOARD;
                }
                OPT_TIME++;

                cout << "--------Optimization goal!!!!!!--------" << endl;
                cout << "---------------opt_time : " << OPT_TIME << "----------------" << endl;
                FINISH_GENERATION = gene; // 最適解到達世代を保存
                break;
            }
        }
        IDV.clear();
    }
}

// ピクロス盤面の確定マスの選定
void Optimization::GameBase()
{
    // 黒マスの確定マスの選出
    for (int i = 0; i < NUM_LINE; i++)
    { // 実行可能行の構築
        long candidate_no = 0;
        vector<int> current_no_set, end_no_set;
        current_no_set.resize(GB.HINTS_LINE[i].size());
        end_no_set.resize(GB.HINTS_LINE[i].size());
        for (int j = 0; j < GB.HINTS_LINE[i].size(); j++)
        { // 実行可能行の開始黒マス要素値
            if (j == 0)
            {
                current_no_set[j] = 0;
            }
            else
            {
                current_no_set[j] = current_no_set[j - 1] + GB.HINTS_LINE[i][j - 1] + 1;
            }
        }

        for (int j = GB.HINTS_LINE[i].size() - 1; j >= 0; j--)
        { // 実行可能行の終了オーバー黒マス要素値
            if (j == GB.HINTS_LINE[i].size() - 1)
            {
                end_no_set[j] = NUM_COL - GB.HINTS_LINE[i][j] + 1;
            }
            else
            {
                end_no_set[j] = end_no_set[j + 1] - GB.HINTS_LINE[i][j] - 1;
            }
        }
        bool roop_flg = false;
        while (roop_flg == false)
        {
            GB.FEASIBLE_LINE[i].emplace_back(candidate_no); // 実行可能行の追加と初期化
            GB.FEASIBLE_LINE[i][candidate_no].resize(NUM_COL);

            for (int j = 0; j < GB.HINTS_LINE[i].size(); j++)
            { // 実行可能行の構築
                for (int k = current_no_set[j]; k < current_no_set[j] + GB.HINTS_LINE[i][j]; k++)
                {
                    GB.FEASIBLE_LINE[i][candidate_no][k] = 1;
                }
            }

            roop_flg = true; // 実行可能行の要素値の加算
            for (int j = GB.HINTS_LINE[i].size() - 1; j >= 0; j--)
            {
                if ((current_no_set[j] + 1) < end_no_set[j])
                {
                    current_no_set[j]++;
                    for (int k = j + 1; k < GB.HINTS_LINE[i].size(); k++)
                    {
                        current_no_set[k] = current_no_set[k - 1] + GB.HINTS_LINE[i][k - 1] + 1;
                    }
                    roop_flg = false;
                    break;
                }
            }
            candidate_no++;
        }
        current_no_set.clear();
        end_no_set.clear();
    }

    for (int i = 0; i < NUM_LINE; i++)
    {
        if (GB.FEASIBLE_LINE[i].empty())
        {
            cout << "Warning: FEASIBLE_LINE[" << i << "] is empty!" << endl;
            continue; // 空ならスキップして安全に処理を飛ばす
        }

        for (int j = 0; j < NUM_COL; j++)
        {
            for (int k = 0; k < GB.FEASIBLE_LINE[i].size(); k++)
            {
                GB.BLACK_PROBABILITY_LINE[i][j] += GB.FEASIBLE_LINE[i][k][j];
            }
            GB.BLACK_PROBABILITY_LINE[i][j] /= GB.FEASIBLE_LINE[i].size();
        }
    }

    vector<bool> feasible_line_table;
    for (int i = 0; i < NUM_LINE; i++)
    { // 実行可能行の確率的選択による確率的実行可能行の構築
        feasible_line_table.resize(GB.FEASIBLE_LINE[i].size());
        for (int k = 0; k < GB.FEASIBLE_LINE[i].size(); k++)
        {
            feasible_line_table[k] = true;
        }
        for (int j = 0; j < NUM_COL; j++)
        {
            if (GB.BLACK_PROBABILITY_LINE[i][j] >= PROBABILITY_LOWER_LIMIT_LINE && GB.BLACK_PROBABILITY_LINE[i][j] < 1.0)
            {
                for (int k = 0; k < GB.FEASIBLE_LINE[i].size(); k++)
                {
                    if (GB.FEASIBLE_LINE[i][k][j] == 0)
                    {
                        feasible_line_table[k] = false;
                    }
                }
            }
        }
        int candidate_no_tmp = 0;
        for (int k = 0; k < GB.FEASIBLE_LINE[i].size(); k++)
        {
            if (feasible_line_table[k] == true)
            {
                GB.PROBABILITY_FEASIBLE_LINE[i].emplace_back(candidate_no_tmp); // 確率的実行可能行の追加と実行可能行のコピー
                GB.PROBABILITY_FEASIBLE_LINE[i][candidate_no_tmp].resize(NUM_COL);
                GB.PROBABILITY_FEASIBLE_LINE[i][candidate_no_tmp] = GB.FEASIBLE_LINE[i][k];
                candidate_no_tmp++;
            }
        }
    }
    feasible_line_table.clear();

    for (int i = 0; i < NUM_COL; i++)
    { // 実行可能列の構築
        long candidate_no = 0;
        vector<int> current_no_set, end_no_set;
        current_no_set.resize(GB.HINTS_COLUMN[i].size());
        end_no_set.resize(GB.HINTS_COLUMN[i].size());

        for (int j = 0; j < GB.HINTS_COLUMN[i].size(); j++)
        { // 実行可能列の開始黒マス要素値
            if (j == 0)
            {
                current_no_set[j] = 0;
            }
            else
            {
                current_no_set[j] = current_no_set[j - 1] + GB.HINTS_COLUMN[i][j - 1] + 1;
            }
        }

        for (int j = GB.HINTS_COLUMN[i].size() - 1; j >= 0; j--)
        { // 実行可能列の終了オーバー黒マス要素値
            if (j == GB.HINTS_COLUMN[i].size() - 1)
            {
                end_no_set[j] = NUM_COL - GB.HINTS_COLUMN[i][j] + 1;
            }
            else
            {
                end_no_set[j] = end_no_set[j + 1] - GB.HINTS_COLUMN[i][j] - 1;
            }
        }
        bool roop_flg = false;
        while (roop_flg == false)
        {
            GB.FEASIBLE_COLUMN[i].emplace_back(candidate_no); // 実行可能列の追加と初期化
            GB.FEASIBLE_COLUMN[i][candidate_no].resize(NUM_LINE);
            for (int j = 0; j < NUM_LINE; j++)
            {
                GB.FEASIBLE_COLUMN[i][candidate_no][j] = 0;
            }

            for (int j = 0; j < GB.HINTS_COLUMN[i].size(); j++)
            { // 実行可能列の構築
                for (int k = current_no_set[j]; k < current_no_set[j] + GB.HINTS_COLUMN[i][j]; k++)
                {
                    GB.FEASIBLE_COLUMN[i][candidate_no][k] = 1;
                }
            }

            roop_flg = true; // 実行可能列の要素値の加算
            for (int j = GB.HINTS_COLUMN[i].size() - 1; j >= 0; j--)
            {
                if ((current_no_set[j] + 1) < end_no_set[j])
                {
                    current_no_set[j]++;
                    for (int k = j + 1; k < GB.HINTS_COLUMN[i].size(); k++)
                    {
                        current_no_set[k] = current_no_set[k - 1] + GB.HINTS_COLUMN[i][k - 1] + 1;
                    }
                    roop_flg = false;
                    break;
                }
            }
            candidate_no++;
        }
        current_no_set.clear();
        end_no_set.clear();
    }

    for (int i = 0; i < NUM_COL; i++)
    { // 列の黒マスの確率
        for (int j = 0; j < NUM_LINE; j++)
        {
            for (int k = 0; k < GB.FEASIBLE_COLUMN[i].size(); k++)
            {
                GB.BLACK_PROBABILITY_COLUMN[j][i] += GB.FEASIBLE_COLUMN[i][k][j];
            }
            GB.BLACK_PROBABILITY_COLUMN[j][i] /= GB.FEASIBLE_COLUMN[i].size();
        }
    }

    // 確定マスの設定
    int black_cell_fixnum = 0;

    if (FIXCELL_FLAG == 1)
    {
        for (int i = 0; i < NUM_LINE; i++)
        {
            for (int j = 0; j < NUM_COL; j++)
            {
                if (GB.BLACK_PROBABILITY_LINE[i][j] == 1)
                {
                    GB.BOARD_FIX_CELL[i][j] = 1;
                    GB.BOARD_FIX_CELL_TMP[i][j] = 1;
                    black_cell_fixnum++;
                    GB.BLACKCELLNUM_LINE[i]++;
                }
                else if (GB.BLACK_PROBABILITY_COLUMN[i][j] == 1)
                {
                    GB.BOARD_FIX_CELL[i][j] = 1;
                    GB.BOARD_FIX_CELL_TMP[i][j] = 1;
                    black_cell_fixnum++;
                    GB.BLACKCELLNUM_COLUMN[i]++;
                }

                // 白の確定
                if (GB.BLACK_PROBABILITY_LINE[i][j] == 0)
                {
                    GB.BOARD_FIX_CELL[i][j] = 2;
                }
                else if (GB.BLACK_PROBABILITY_COLUMN[i][j] == 0)
                {
                    GB.BOARD_FIX_CELL[i][j] = 2;
                }
            }
        }
    }

    // 確定黒マスの配置
    for (int i = 0; i < NUM_LINE; i++)
    {
        for (int j = 0; j < NUM_COL; j++)
        {
            cout << GB.BOARD_FIX_CELL[i][j] << ",";
        }
        cout << endl;
    }

    cout << "fix num:" << black_cell_fixnum << endl;

    // 実行可能行、列の合計値の算出

    int total_feasible_line = 0;
    int total_feasible_col = 0;

    for (int i = 0; i < GB.FEASIBLE_LINE.size(); ++i)
    {
        int copy_line = GB.FEASIBLE_LINE[i].size();
        total_feasible_line += copy_line;
    }

    for (int i = 0; i < GB.FEASIBLE_COLUMN.size(); ++i)
    {
        int copy_col = GB.FEASIBLE_COLUMN[i].size();
        total_feasible_col += copy_col;
    }

    cout << "total feasible line num : " << total_feasible_line << endl;
    cout << "total feasible column num : " << total_feasible_col << endl;
    cout << "picross base OK" << endl;

    // 黒マス配置数の算出
    int total_cell_line = 0;
    int total_cell_col = 0;

    for (int i = 0; i < NUM_LINE; i++)
    {
        for (int j = 0; j < GB.HINTS_LINE[i].size(); j++)
        {
            total_cell_line += GB.HINTS_LINE[i][j];
            GB.BLACKCELLNUM_LINE[i] += GB.HINTS_LINE[i][j];
        }
    }

    for (int i = 0; i < NUM_COL; i++)
    {
        for (int j = 0; j < GB.HINTS_COLUMN[i].size(); j++)
        {
            total_cell_col += GB.HINTS_COLUMN[i][j];
            GB.BLACKCELLNUM_COLUMN[i] += GB.HINTS_COLUMN[i][j];
        }
    }
    if (total_cell_col == total_cell_line)
    {
        cout << "cellcount_OK" << endl;
        cout << "black num" << total_cell_col;
    }
    NUM_CELL = total_cell_line - black_cell_fixnum;

    // 行ごとの黒マス数の算出
}

// 初期化（初期盤面の作成）
void Optimization::Initialization(int loop)
{

    IDV.resize(PARENTS_SIZE + OFFSPRING_SIZE); // 解の構造体の初期化

    for (int i = 0; i < PARENTS_SIZE + OFFSPRING_SIZE; i++)
    {
        IDV[i].BOARD.resize(NUM_LINE);
        for (int j = 0; j < NUM_LINE; j++)
        {
            IDV[i].BOARD[j].resize(NUM_COL);
        }
        IDV[i].EVALUATION_VALUE_COLUMN.resize(NUM_COL);
        IDV[i].EVALUATION_VALUE_LINE.resize(NUM_LINE);
    }

    cout << "cell_num : " << NUM_CELL << endl;

    for (int i = 0; i < PARENTS_SIZE; ++i)
    {
        IDV[i].BOARD = GB.BOARD_FIX_CELL_TMP;
    }

    // 初期集団の形成
    for (int i = 0; i < PARENTS_SIZE; ++i)
    {
        int seed = generate_seed(loop, 0, i);
        mt19937 rng(seed);
        uniform_int_distribution<int> dist_line(0, NUM_LINE - 1);
        uniform_int_distribution<int> dist_col(0, NUM_COL - 1);

        // 確定マスをコピー
        IDV[i].BOARD = GB.BOARD_FIX_CELL_TMP;

        if (INITIALIZATION_FLAG == 0)
        {
            int black_cell_counter_copy = NUM_CELL;

            while (black_cell_counter_copy > 0)
            {
                int line = dist_line(rng); // もしランダムで行も選ぶ場合だけ
                int column = dist_col(rng);

                if (IDV[i].BOARD[line][column] == 0 && GB.BOARD_FIX_CELL[line][column] == 0)
                {
                    IDV[i].BOARD[line][column] = 1;
                    black_cell_counter_copy--;
                }
            }
        }
        else if (INITIALIZATION_FLAG == 1)
        {
            // 行ごとの制約付き初期化
            for (int line = 0; line < NUM_LINE; ++line)
            {
                int total_needed = 0;
                for (int h : GB.HINTS_LINE[line])
                    total_needed += h;

                int fixed_black = GB.BLACKCELLFIXNUM_LINE[line];
                int need_to_place = total_needed - fixed_black;
                if (need_to_place <= 0)
                    continue;

                vector<int> candidate_columns;
                for (int column = 0; column < NUM_COL; ++column)
                {
                    if (GB.BOARD_FIX_CELL[line][column] == 0)
                        candidate_columns.push_back(column);
                }

                if (candidate_columns.size() < need_to_place)
                    continue;

                shuffle(candidate_columns.begin(), candidate_columns.end(), rng);

                for (int k = 0; k < need_to_place; ++k)
                    IDV[i].BOARD[line][candidate_columns[k]] = 1;
            }
        }

        EvaluationFunction_LineCol(i); // 評価関数は必ずここで
    }
}

void Optimization::Crossover(int loop)
{
    mt19937 rng(BASE_SEED + loop * LOOP_OFFSET + OPERATION_OFFSET * 2 + INDIVIDUAL_OFFSET);
    // 領域交叉

    // 親の決定
    uniform_int_distribution<int> dist_line(0, NUM_LINE - 1);
    uniform_int_distribution<int> dist_col(0, NUM_COL - 1);
    uniform_int_distribution<int> dist_parent(0, PARENTS_SIZE - 1);

    //------親の選択方法をもし恣意的変えるならばこっちを採用したい------

    int parent1 = 0, parent2 = 0;
    int offspring_no = 0;

    int crossover_flag = 0;

    // 交叉点の設定　(r_1 < r_2となるように設定)
    int r_1 = dist_line(rng);
    int r_2 = dist_line(rng);

    if (r_1 > r_2)
    {
        swap(r_1, r_2);
    }

    int c_1 = dist_col(rng);
    int c_2 = dist_col(rng);

    if (c_1 > c_2)
    {
        swap(c_1, c_2);
    }

    for (int i = 0; i < PARENTS_SIZE; i++)
    {
        // 親選択の方法（異なるサブ手段から1つずつ選択する）
        if (PARENTS_FLAG == 0)
        {
            parent1 = i;
            parent2 = dist_parent(rng);
            while (parent1 == parent2)
            {
                parent2 = parent2 = dist_parent(rng);
            }
        }

        // 異なるサブ集団について実施したい場合
        if (PARENTS_FLAG == 1)
        {
            parent1 = i;
            parent2 = dist_parent(rng);
            // 同個体か同サブ集団とならないように設定
            while (parent1 == parent2 || (parent1 % 2 == parent2 % 2))
            {
                parent2 = dist_parent(rng);
            }
        }

        // サブ集団の相手をトーナメント選択で選びたい場合
        if (PARENTS_FLAG == 2)
        {
            parent1 = i;

            vector<int> candidate_pool;
            for (int j = 0; j < PARENTS_SIZE; j++)
            {
                if ((i % 2 == 0 && j % 2 == 1) || (i % 2 == 1 && j % 2 == 0))
                {
                    candidate_pool.push_back(j);
                }
            }

            // シャッフルして上位50%だけを選ぶ
            shuffle(candidate_pool.begin(), candidate_pool.end(), mt);
            int tournament_size = candidate_pool.size() / 2;

            // トーナメント選抜（目的関数 f1 による）
            int best = candidate_pool[0];
            double best_fitness = IDV[best].EVALUATION_VALUE_F_1;

            for (int t = 1; t < tournament_size; t++)
            {
                int idx = candidate_pool[t];
                if (IDV[idx].EVALUATION_VALUE_F_1 < best_fitness)
                {
                    best = idx;
                    best_fitness = IDV[idx].EVALUATION_VALUE_F_1;
                }
            }

            parent2 = best;
        }

        // ルーレット選択により選びたい場合
        if (PARENTS_FLAG == 3)
        {
            parent1 = i;

            vector<int> candidate_pool;
            for (int j = 0; j < PARENTS_SIZE; j++)
            {
                if ((i % 2 == 0 && j % 2 == 1) || (i % 2 == 1 && j % 2 == 0))
                {
                    candidate_pool.push_back(j);
                }
            }

            // 適応度の計算（目的関数 f1 に基づく）
            vector<double> fitness(candidate_pool.size());
            double total_fitness = 0.0;
            for (int t = 0; t < candidate_pool.size(); t++)
            {
                fitness[t] = 1.0 / (1.0 + IDV[candidate_pool[t]].EVALUATION_VALUE_F_1); // 適応度の計算
                total_fitness += fitness[t];
            }

            // ルーレット選択
            uniform_real_distribution<double> dist(0.0, total_fitness);
            double rand_value = dist(rng);
            double cumulative_fitness = 0.0;
            for (int t = 0; t < candidate_pool.size(); t++)
            {
                cumulative_fitness += fitness[t];
                if (cumulative_fitness >= rand_value)
                {
                    parent2 = candidate_pool[t];
                    break;
                }
            }
        }

        // 盤面の複製
        IDV[PARENTS_SIZE + offspring_no].BOARD = IDV[parent1].BOARD;
        IDV[PARENTS_SIZE + offspring_no + 1].BOARD = IDV[parent2].BOARD;

        // 領域交叉
        if (CROSSOVER_FLAG == 0)
        {
            for (int j = r_1; j <= r_2; j++)
            {
                for (int k = c_1; k <= c_2; k++)
                {
                    swap(IDV[PARENTS_SIZE + offspring_no].BOARD[j][k], IDV[PARENTS_SIZE + offspring_no + 1].BOARD[j][k]);
                }
            }
        }

        // 2点交叉（行方向）
        if (CROSSOVER_FLAG == 1)
        {
            for (int j = r_1; j <= r_2; j++)
            {
                swap(IDV[PARENTS_SIZE + offspring_no].BOARD[j], IDV[PARENTS_SIZE + offspring_no + 1].BOARD[j]);
            }
        }

        // 半数を行交叉、半数を列交叉
        if (CROSSOVER_FLAG == 2)
        {
            // 交叉方法の設定（半分を行交叉、半分を列交叉）
            crossover_flag = i % 2;
            // 2点交叉（行方向）
            if (crossover_flag == 0)
            {
                for (int j = r_1; j <= r_2; j++)
                {
                    swap(IDV[PARENTS_SIZE + offspring_no].BOARD[j], IDV[PARENTS_SIZE + offspring_no + 1].BOARD[j]);
                }
            }

            // 2点交叉(列方向)
            if (crossover_flag == 1)
            {
                for (int i = 0; i < NUM_LINE; i++)
                {
                    for (int j = c_1; j <= c_2; j++)
                    {
                        swap(IDV[PARENTS_SIZE + offspring_no].BOARD[i][j], IDV[PARENTS_SIZE + offspring_no + 1].BOARD[i][j]);
                    }
                }
            }
        }

        Adjust_blackcell(PARENTS_SIZE + offspring_no, r_1, r_2, c_1, c_2, loop);
        Adjust_blackcell(PARENTS_SIZE + offspring_no + 1, r_1, r_2, c_1, c_2, loop);

        EvaluationFunction_LineCol(PARENTS_SIZE + offspring_no);
        EvaluationFunction_LineCol(PARENTS_SIZE + offspring_no + 1);

        offspring_no += 2;
    }
}

void Optimization::Mutation(int gene, int loop)
{
    mt19937 rng(BASE_SEED + loop * LOOP_OFFSET + OPERATION_OFFSET * 2 + gene * INDIVIDUAL_OFFSET);
    uniform_real_distribution<double> dist_prob(0.0, 1.0); // 変異率の設定

    // 交叉後 offspring にのみ突然変異を適用
    for (int i = 0; i < OFFSPRING_SIZE; i++)
    {
        int idx = PARENTS_SIZE + i; // ★ 突然変異対象（交叉個体）

        if (dist_prob(rng) > MUTATION_RATE)
        {
            // 変異しない → 交叉結果をそのまま使う
            EvaluationFunction_LineCol(idx);
            continue;
        }

        // 盤面はすでに Crossover() で作られているのでコピー不要

        uniform_int_distribution<int> dist_line(0, NUM_LINE - 1);
        uniform_int_distribution<int> dist_col(0, NUM_COL - 1);

        int r1 = dist_line(rng);
        int r2 = dist_line(rng);
        if (r1 > r2)
            swap(r1, r2);

        int c1 = dist_col(rng);
        int c2 = dist_col(rng);
        if (c1 > c2)
            swap(c1, c2);

        int black_count = 0;

        if (MUTATION_FLAG == 0)
        {
            // 黒マスの数を数える
            for (int row = r1; row <= r2; row++)
            {
                for (int col = c1; col <= c2; ++col)
                {
                    if (IDV[idx].BOARD[row][col] == 1)
                    {
                        black_count++;
                    }
                    // 一旦すべて初期化（0）
                    IDV[idx].BOARD[row][col] = 0;
                }
            }
            // 黒マス配置の範囲設定
            uniform_int_distribution<int> black_line(r1, r2);
            uniform_int_distribution<int> black_col(c1, c2);

            while (black_count > 0)
            {
                int choice_line = black_line(rng);
                int choice_col = black_col(rng);

                if (IDV[idx].BOARD[choice_line][choice_col] == 0)
                {
                    IDV[idx].BOARD[choice_line][choice_col] = 1;
                    black_count--;
                }
            }
        }

        if (MUTATION_FLAG == 1)
        {
            // 黒マスの数を数える
            for (int row = r1; row <= r2; row++)
            {
                for (int col = c1; col <= c2; ++col)
                {
                    if (IDV[idx].BOARD[row][col] == 1)
                    {
                        black_count++;
                    }
                    // 一旦すべて初期化（0）
                    IDV[idx].BOARD[row][col] = 0;
                    // 確定マスはすでに定義
                    if (GB.BOARD_FIX_CELL[row][col] == 1)
                    {
                        IDV[idx].BOARD[row][col] = 1;
                        black_count--;
                    }
                }
            }

            // 黒マス配置の範囲設定
            uniform_int_distribution<int> black_line(r1, r2);
            uniform_int_distribution<int> black_col(c1, c2);

            while (black_count > 0)
            {
                int choice_line = black_line(rng);
                int choice_col = black_col(rng);

                if (IDV[idx].BOARD[choice_line][choice_col] == 0 && GB.BOARD_FIX_CELL[choice_line][choice_col] != 1)
                {
                    IDV[idx].BOARD[choice_line][choice_col] = 1;
                    black_count--;
                }
            }
        }

        if (MUTATION_FLAG == 2)
        {
            int mutation_pass = i % 2;
            if (mutation_pass == 0)
            {
                for (int row = r1; row <= r2; row++)
                {
                    for (int col = 0; col <= NUM_COL; col++)
                    {
                        if (IDV[idx].BOARD[row][col] == 1)
                        {
                            black_count++;
                        }
                        // 一旦すべて初期化（0）
                        IDV[idx].BOARD[row][col] = 0;
                        // 確定マスはすでに定義
                        if (GB.BOARD_FIX_CELL[row][col] == 1)
                        {
                            IDV[idx].BOARD[row][col] = 1;
                            black_count--;
                        }
                    }
                }
                // 黒マス配置の範囲設定
                uniform_int_distribution<int> black_line(r1, r2);
                uniform_int_distribution<int> black_col(0, NUM_COL);

                while (black_count > 0)
                {
                    int choice_line = black_line(rng);
                    int choice_col = black_col(rng);

                    if (IDV[idx].BOARD[choice_line][choice_col] == 0 && GB.BOARD_FIX_CELL[choice_line][choice_col] != 1)
                    {
                        IDV[idx].BOARD[choice_line][choice_col] = 1;
                        black_count--;
                    }
                }
            }
            if (mutation_pass == 1)
            {
                for (int col = c1; col <= c2; col++)
                {
                    for (int row = 0; row <= NUM_LINE; row++)
                    {
                        if (IDV[idx].BOARD[row][col] == 1)
                        {
                            black_count++;
                        }
                        // 一旦すべて初期化（0）
                        IDV[idx].BOARD[row][col] = 0;
                        // 確定マスはすでに定義
                        if (GB.BOARD_FIX_CELL[row][col] == 1)
                        {
                            IDV[idx].BOARD[row][col] = 1;
                            black_count--;
                        }
                    }
                }
                // 黒マス配置の範囲設定
                uniform_int_distribution<int> black_line(0, NUM_LINE);
                uniform_int_distribution<int> black_col(c1, c2);

                while (black_count > 0)
                {
                    int choice_line = black_line(rng);
                    int choice_col = black_col(rng);

                    if (IDV[idx].BOARD[choice_line][choice_col] == 0 && GB.BOARD_FIX_CELL[choice_line][choice_col] != 1)
                    {
                        IDV[idx].BOARD[choice_line][choice_col] = 1;
                        black_count--;
                    }
                }
            }
        }

        if (MUTATION_FLAG == 3)
        {
            // 1. 行または列を選択（50%ずつ）
            uniform_int_distribution<int> line_or_col(0, 1);
            bool mutate_line = line_or_col(rng);

            int target_index = -1;

            // 2. 違反の大きい行または列を選択
            if (mutate_line)
            {
                vector<pair<long, int>> line_eval;
                for (int j = 0; j < NUM_LINE; j++)
                    line_eval.push_back({IDV[idx].EVALUATION_VALUE_LINE[j], j});
                sort(line_eval.rbegin(), line_eval.rend());
                target_index = line_eval.front().second;
            }
            else
            {
                vector<pair<long, int>> col_eval;
                for (int j = 0; j < NUM_COL; j++)
                    col_eval.push_back({IDV[idx].EVALUATION_VALUE_COLUMN[j], j});
                sort(col_eval.rbegin(), col_eval.rend());
                target_index = col_eval.front().second;
            }

            // 3. 実行可能配列からランダムに1つ選択し置き換え
            if (mutate_line)
            {
                uniform_int_distribution<int> feasible_dist(0, GB.FEASIBLE_LINE[target_index].size() - 1);
                int pattern_idx = feasible_dist(rng);

                // 行をそのまま置き換え
                IDV[idx].BOARD[target_index] = GB.FEASIBLE_LINE[target_index][pattern_idx];
            }
            else
            {
                uniform_int_distribution<int> feasible_dist(0, GB.FEASIBLE_COLUMN[target_index].size() - 1);
                int pattern_idx = feasible_dist(rng);

                // 列の置き換え（固定マスは変更しない）
                for (int r = 0; r < NUM_LINE; r++)
                {
                    if (GB.BOARD_FIX_CELL[r][target_index] != 1)
                        IDV[idx].BOARD[r][target_index] = GB.FEASIBLE_COLUMN[target_index][pattern_idx][r];
                }
            }
        }

        if (MUTATION_FLAG == 4) // 行のみの置換
        {
            // 違反が大きい行を選択
            int target_index = -1;
            vector<pair<long, int>> line_eval;
            for (int j = 0; j < NUM_LINE; j++)
                line_eval.push_back({IDV[idx].EVALUATION_VALUE_LINE[j], j});
            sort(line_eval.rbegin(), line_eval.rend());
            target_index = line_eval.front().second;

            // 実行可能パターンをランダムに1つ選ぶ
            uniform_int_distribution<int> feasible_dist(0, GB.FEASIBLE_LINE[target_index].size() - 1);
            int pattern_idx = feasible_dist(rng);

            // 固定マスを壊さずに再配置
            for (int col = 0; col < NUM_COL; col++)
            {
                if (GB.BOARD_FIX_CELL[target_index][col] != 1)
                {
                    IDV[idx].BOARD[target_index][col] =
                        GB.FEASIBLE_LINE[target_index][pattern_idx][col];
                }
            }
        }

        if (MUTATION_FLAG == 5)
        {
            // 評価値が最も悪い行 or 列を選択
            bool use_line = true;
            int target = -1;

            long max_line = -1, max_col = -1;
            int worst_line = -1, worst_col = -1;

            for (int r = 0; r < NUM_LINE; r++)
            {
                if (IDV[idx].EVALUATION_VALUE_LINE[r] > max_line)
                {
                    max_line = IDV[idx].EVALUATION_VALUE_LINE[r];
                    worst_line = r;
                }
            }

            for (int c = 0; c < NUM_COL; c++)
            {
                if (IDV[idx].EVALUATION_VALUE_COLUMN[c] > max_col)
                {
                    max_col = IDV[idx].EVALUATION_VALUE_COLUMN[c];
                    worst_col = c;
                }
            }

            if (max_col > max_line)
            {
                use_line = false;
                target = worst_col;
            }
            else
            {
                use_line = true;
                target = worst_line;
            }

            // ペア反転を3回実施（黒マス数保存・確定マス除外）
            const int SWAP_K = 3;

            for (int rep = 0; rep < SWAP_K; rep++)
            {
                vector<pair<int, int>> ones;
                vector<pair<int, int>> zeros;

                if (use_line)
                {
                    int r = target;
                    for (int c = 0; c < NUM_COL; c++)
                    {
                        if (GB.BOARD_FIX_CELL[r][c] != 0)
                            continue;

                        if (IDV[idx].BOARD[r][c] == 1)
                            ones.emplace_back(r, c);
                        else
                            zeros.emplace_back(r, c);
                    }
                }
                else
                {
                    int c = target;
                    for (int r = 0; r < NUM_LINE; r++)
                    {
                        if (GB.BOARD_FIX_CELL[r][c] != 0)
                            continue;

                        if (IDV[idx].BOARD[r][c] == 1)
                            ones.emplace_back(r, c);
                        else
                            zeros.emplace_back(r, c);
                    }
                }

                if (ones.empty() || zeros.empty())
                    break;

                uniform_int_distribution<int> dist1(0, ones.size() - 1);
                uniform_int_distribution<int> dist0(0, zeros.size() - 1);

                auto [r1, c1] = ones[dist1(rng)];
                auto [r2, c2] = zeros[dist0(rng)];

                // 1↔0 ペア反転（黒マス数保存）
                IDV[idx].BOARD[r1][c1] = 0;
                IDV[idx].BOARD[r2][c2] = 1;
            }
        }

        // 評価関数の再計算
        EvaluationFunction_LineCol(idx);
    }
}

// 局所探索アルゴリズム（違反誘導型・2点反転・first improvement）
void Optimization::LocalSearch(int idx)
{
    if (GA_FLAG != 1)
        return;

    cout << "[LS start] idx=" << idx
         << " f=(" << IDV[idx].EVALUATION_VALUE_F_1
         << "," << IDV[idx].EVALUATION_VALUE_F_2 << ")" << endl;

    long best_f1 = IDV[idx].EVALUATION_VALUE_F_1;
    long best_f2 = IDV[idx].EVALUATION_VALUE_F_2;
    long best_val = best_f1 * best_f1 + best_f2 * best_f2;

    mt19937 rng(BASE_SEED + idx * 131);

    // 違反が最大の行 or 列を選択
    bool use_line = true;
    int target = -1;

    long max_line = -1, max_col = -1;
    int worst_line = -1, worst_col = -1;

    for (int i = 0; i < NUM_LINE; i++)
    {
        if (IDV[idx].EVALUATION_VALUE_LINE[i] > max_line)
        {
            max_line = IDV[idx].EVALUATION_VALUE_LINE[i];
            worst_line = i;
        }
    }

    for (int j = 0; j < NUM_COL; j++)
    {
        if (IDV[idx].EVALUATION_VALUE_COLUMN[j] > max_col)
        {
            max_col = IDV[idx].EVALUATION_VALUE_COLUMN[j];
            worst_col = j;
        }
    }

    if (max_col > max_line)
    {
        use_line = false;
        target = worst_col;
    }
    else
    {
        use_line = true;
        target = worst_line;
    }

    // 反転候補の収集
    vector<pair<int, int>> ones;
    vector<pair<int, int>> zeros;

    if (use_line)
    {
        int r = target;
        for (int c = 0; c < NUM_COL; c++)
        {
            if (GB.BOARD_FIX_CELL[r][c] == 1)
                continue;

            if (IDV[idx].BOARD[r][c] == 1)
                ones.emplace_back(r, c);
            else
                zeros.emplace_back(r, c);
        }
    }
    else
    {
        int c = target;
        for (int r = 0; r < NUM_LINE; r++)
        {
            if (GB.BOARD_FIX_CELL[r][c] == 1)
                continue;

            if (IDV[idx].BOARD[r][c] == 1)
                ones.emplace_back(r, c);
            else
                zeros.emplace_back(r, c);
        }
    }

    if (ones.empty() || zeros.empty())
    {
        cout << "[LS skipped] no valid flip candidates" << endl;
        return;
    }

    vector<vector<int>> backup = IDV[idx].BOARD;

    uniform_int_distribution<int> dist1(0, ones.size() - 1);
    uniform_int_distribution<int> dist0(0, zeros.size() - 1);

    // 試行
    for (int trial = 0; trial < LS_MAX_TRIAL; trial++)
    {
        auto [r1, c1] = ones[dist1(rng)];
        auto [r2, c2] = zeros[dist0(rng)];

        // 2点反転（黒数保存）
        IDV[idx].BOARD[r1][c1] = 0;
        IDV[idx].BOARD[r2][c2] = 1;

        EvaluationFunction_LineCol(idx);

        long f1 = IDV[idx].EVALUATION_VALUE_F_1;
        long f2 = IDV[idx].EVALUATION_VALUE_F_2;
        long val = f1 * f1 + f2 * f2;

        if (val < best_val)
        {
            cout << "[LS improved] idx=" << idx
                 << " (" << best_f1 << "," << best_f2
                 << " -> " << f1 << "," << f2 << ")" << endl;
            return;
        }

        // rollback
        IDV[idx].BOARD = backup;
        IDV[idx].EVALUATION_VALUE_F_1 = best_f1;
        IDV[idx].EVALUATION_VALUE_F_2 = best_f2;
    }

    cout << "[LS no improvement] idx=" << idx
         << " f=(" << best_f1 << "," << best_f2 << ")" << endl;
}

void Optimization::Adjust_blackcell(int i, int r1, int r2, int c1, int c2, int loop)
{
    int counter = 0;
    for (int row = r1; row <= r2; ++row)
    {
        for (int col = c1; col <= c2; ++col)
        {
            if (IDV[i].BOARD[row][col] == 1)
                counter++;
        }
    }

    int diff = NUM_CELL - counter; // 黒マスの差分（増やすなら正、減らすなら負）

    mt19937 rng(loop); // シードを固定
    uniform_int_distribution<int> dist_line(r1, r2);
    uniform_int_distribution<int> dist_col(c1, c2);

    if (diff > 0)
    {
        // 0マスだけ抽出（1に変える候補）
        vector<vector<int>> zero_cells;
        for (int row = r1; row <= r2; ++row)
        {
            for (int col = c1; col <= c2; ++col)
            {
                if (IDV[i].BOARD[row][col] == 0 && GB.BOARD_FIX_CELL[row][col] != 1)
                {
                    zero_cells.push_back({row, col});
                }
            }
        }

        // 候補がないなら何もしない
        if (zero_cells.empty())
            return;

        shuffle(zero_cells.begin(), zero_cells.end(), rng);

        int count = 0;
        for (auto &cell : zero_cells)
        {
            if (count >= diff)
                break;
            int row = cell[0];
            int col = cell[1];
            IDV[i].BOARD[row][col] = 1;
            count++;
        }
    }
    else if (diff < 0)
    {
        // 1マスだけ抽出（0に変える候補）
        vector<vector<int>> one_cells;
        for (int row = r1; row <= r2; ++row)
        {
            for (int col = c1; col <= c2; ++col)
            {
                if (IDV[i].BOARD[row][col] == 1 && GB.BOARD_FIX_CELL[row][col] != 1)
                {
                    one_cells.push_back({row, col});
                }
            }
        }

        if (one_cells.empty())
            return;

        shuffle(one_cells.begin(), one_cells.end(), rng);

        int count = 0;
        for (auto &cell : one_cells)
        {
            if (count >= -diff)
                break;
            int row = cell[0];
            int col = cell[1];
            IDV[i].BOARD[row][col] = 0;
            count++;
        }
    }
}

int Optimization::Selection_Elite(int gene)
{
    int pop_size = IDV.size();
    int elite_size = PARENTS_SIZE / 2;
    int BestIndividualNo = 0;
    double min_value = numeric_limits<double>::max();

    if (gene == 0)
    {
        for (int i = 0; i < PARENTS_SIZE; i++)
        {
            double f1 = IDV[i].EVALUATION_VALUE_F_1;
            double f2 = IDV[i].EVALUATION_VALUE_F_2;
            double eval = f1 * f1 + f2 * f2;
        }
    }

    else
    {
        // ソート用
        vector<int> sorted_by_f1(pop_size), sorted_by_f2(pop_size);
        iota(sorted_by_f1.begin(), sorted_by_f1.end(), 0);
        iota(sorted_by_f2.begin(), sorted_by_f2.end(), 0);

        sort(sorted_by_f1.begin(), sorted_by_f1.end(),
             [&](int a, int b)
             { return IDV[a].EVALUATION_VALUE_F_1 < IDV[b].EVALUATION_VALUE_F_1; });

        sort(sorted_by_f2.begin(), sorted_by_f2.end(),
             [&](int a, int b)
             { return IDV[a].EVALUATION_VALUE_F_2 < IDV[b].EVALUATION_VALUE_F_2; });

        // 重複を許容してエリートを選出
        vector<int> copy_choice;
        for (int i = 0; i < elite_size && i < pop_size; ++i)
        {
            copy_choice.push_back(sorted_by_f1[i]);
            copy_choice.push_back(sorted_by_f2[i]);
        }

        // 次世代へのコピー
        vector<vector<vector<int>>> board_tmp; // 次世代集団の更新
        board_tmp.resize(PARENTS_SIZE);

        vector<long> evaluation_value_f1_tmp;
        evaluation_value_f1_tmp.resize(PARENTS_SIZE);

        vector<long> evaluation_value_f2_tmp;
        evaluation_value_f2_tmp.resize(PARENTS_SIZE);

        vector<vector<long>> evaluation_value_line_tmp;
        evaluation_value_line_tmp.resize(PARENTS_SIZE);

        vector<vector<long>> evaluation_value_column_tmp;
        evaluation_value_column_tmp.resize(PARENTS_SIZE);

        for (int i = 0; i < PARENTS_SIZE; i++)
        {
            board_tmp[i].resize(NUM_LINE);
            for (int j = 0; j < NUM_LINE; j++)
            {
                board_tmp[i][j].resize(NUM_COL);
            }
            evaluation_value_line_tmp[i].resize(NUM_LINE);
            evaluation_value_column_tmp[i].resize(NUM_COL);
        }

        for (int i = 0; i < PARENTS_SIZE; i++)
        {
            board_tmp[i] = IDV[copy_choice[i]].BOARD;
            evaluation_value_line_tmp[i] = IDV[copy_choice[i]].EVALUATION_VALUE_LINE;
            evaluation_value_column_tmp[i] = IDV[copy_choice[i]].EVALUATION_VALUE_COLUMN;
            evaluation_value_f1_tmp[i] = IDV[copy_choice[i]].EVALUATION_VALUE_F_1;
            evaluation_value_f2_tmp[i] = IDV[copy_choice[i]].EVALUATION_VALUE_F_2;
        }

        for (int i = 0; i < PARENTS_SIZE; i++)
        {
            IDV[i].BOARD = board_tmp[i];
            IDV[i].EVALUATION_VALUE_LINE = evaluation_value_line_tmp[i];
            IDV[i].EVALUATION_VALUE_COLUMN = evaluation_value_column_tmp[i];

            IDV[i].EVALUATION_VALUE_F_1 = evaluation_value_f1_tmp[i];
            IDV[i].EVALUATION_VALUE_F_2 = evaluation_value_f2_tmp[i];
        }

        board_tmp.clear();
        evaluation_value_line_tmp.clear();
        evaluation_value_column_tmp.clear();
        evaluation_value_f1_tmp.clear();
        evaluation_value_f2_tmp.clear();
    }

    // 最良個体の抽出
    for (int i = 0; i < PARENTS_SIZE; i++)
    {
        double f1 = IDV[i].EVALUATION_VALUE_F_1;
        double f2 = IDV[i].EVALUATION_VALUE_F_2;
        double eval = f1 * f1 + f2 * f2;

        if (eval < min_value)
        {
            min_value = eval;
            BestIndividualNo = i;
        }
    }

    return BestIndividualNo;
}

int Optimization::Selection_tonament(int gene)
{
    int BestIndividualNo = 0; // 最良個体の抽出
    double min_value = numeric_limits<double>::max();

    if (gene == 0)
    { // 初期世代の場合
        long min_value_f_1 = LONG_MAX;
        long min_value_f_2 = LONG_MAX;
        for (int i = 0; i < PARENTS_SIZE; i++)
        {
            if (IDV[i].EVALUATION_VALUE_F_1 < min_value_f_1)
            {
                min_value_f_1 = IDV[i].EVALUATION_VALUE_F_1;
            }
            if (IDV[i].EVALUATION_VALUE_F_2 < min_value_f_2)
            {
                min_value_f_2 = IDV[i].EVALUATION_VALUE_F_2;
            }
        }
    }
    else
    { // 初期世代以降の場合
        int half_size = PARENTS_SIZE / 2;

        vector<int> f1_selected(half_size);
        vector<int> f2_selected(half_size);
        vector<int> copy_choice(PARENTS_SIZE); // 次世代インデックス保存用
        vector<int> tournament_tmp(PARENTS_SIZE + OFFSPRING_SIZE);

        // ---------- F1方向 ----------
        for (int i = 0; i < half_size; i++)
        {
            iota(tournament_tmp.begin(), tournament_tmp.end(), 0);
            shuffle(tournament_tmp.begin(), tournament_tmp.end(), mt);

            long min_val = LONG_MAX;
            int best_idx = -1;

            for (int j = 0; j < TOURNAMENT_SIZE; j++)
            {
                int idx = tournament_tmp[j];
                if (IDV[idx].EVALUATION_VALUE_F_1 < min_val)
                {
                    min_val = IDV[idx].EVALUATION_VALUE_F_1;
                    best_idx = idx;
                }
            }
            f1_selected[i] = best_idx;
        }

        // ---------- F2方向 ----------
        for (int i = 0; i < half_size; i++)
        {
            iota(tournament_tmp.begin(), tournament_tmp.end(), 0);
            shuffle(tournament_tmp.begin(), tournament_tmp.end(), mt);

            long min_val = LONG_MAX;
            int best_idx = -1;

            for (int j = 0; j < TOURNAMENT_SIZE; j++)
            {
                int idx = tournament_tmp[j];
                if (IDV[idx].EVALUATION_VALUE_F_2 < min_val)
                {
                    min_val = IDV[idx].EVALUATION_VALUE_F_2;
                    best_idx = idx;
                }
            }
            f2_selected[i] = best_idx;
        }

        // ---------- 次世代インデックスの交互構成 ----------
        for (int i = 0; i < half_size; i++)
        {
            copy_choice[i * 2] = f1_selected[i];     // 偶数番目にF1
            copy_choice[i * 2 + 1] = f2_selected[i]; // 奇数番目にF2
        }

        // 次世代へのコピー
        vector<vector<vector<int>>> board_tmp; // 次世代集団の更新
        board_tmp.resize(PARENTS_SIZE);

        vector<long> evaluation_value_f1_tmp;
        evaluation_value_f1_tmp.resize(PARENTS_SIZE);

        vector<long> evaluation_value_f2_tmp;
        evaluation_value_f2_tmp.resize(PARENTS_SIZE);

        vector<vector<long>> evaluation_value_line_tmp;
        evaluation_value_line_tmp.resize(PARENTS_SIZE);

        vector<vector<long>> evaluation_value_column_tmp;
        evaluation_value_column_tmp.resize(PARENTS_SIZE);

        for (int i = 0; i < PARENTS_SIZE; i++)
        {
            board_tmp[i].resize(NUM_LINE);
            for (int j = 0; j < NUM_LINE; j++)
            {
                board_tmp[i][j].resize(NUM_COL);
            }
            evaluation_value_line_tmp[i].resize(NUM_LINE);
            evaluation_value_column_tmp[i].resize(NUM_COL);
        }

        for (int i = 0; i < PARENTS_SIZE; i++)
        {
            board_tmp[i] = IDV[copy_choice[i]].BOARD;
            evaluation_value_line_tmp[i] = IDV[copy_choice[i]].EVALUATION_VALUE_LINE;
            evaluation_value_column_tmp[i] = IDV[copy_choice[i]].EVALUATION_VALUE_COLUMN;
            evaluation_value_f1_tmp[i] = IDV[copy_choice[i]].EVALUATION_VALUE_F_1;
            evaluation_value_f2_tmp[i] = IDV[copy_choice[i]].EVALUATION_VALUE_F_2;
        }

        for (int i = 0; i < PARENTS_SIZE; i++)
        {
            IDV[i].BOARD = board_tmp[i];
            IDV[i].EVALUATION_VALUE_LINE = evaluation_value_line_tmp[i];
            IDV[i].EVALUATION_VALUE_COLUMN = evaluation_value_column_tmp[i];

            IDV[i].EVALUATION_VALUE_F_1 = evaluation_value_f1_tmp[i];
            IDV[i].EVALUATION_VALUE_F_2 = evaluation_value_f2_tmp[i];
        }

        board_tmp.clear();
        evaluation_value_line_tmp.clear();
        evaluation_value_column_tmp.clear();
        evaluation_value_f1_tmp.clear();
        evaluation_value_f2_tmp.clear();
    }

    // 最良個体の抽出
    for (int i = 0; i < PARENTS_SIZE; i++)
    {
        long f1 = IDV[i].EVALUATION_VALUE_F_1;
        long f2 = IDV[i].EVALUATION_VALUE_F_2;

        if (f1 == 0 && f2 == 0)
        {
            // 最適解を見つけたら即リターン
            return i;
        }

        int eval = f1 * f1 + f2 * f2;
        if (eval < min_value)
        {
            min_value = eval;
            BestIndividualNo = i;
        }
    }
    return BestIndividualNo;
}

int Optimization::selection_roulette()
{
    // ルーレット選択の実装
    double total_fitness = 0;
    for (int i = 0; i < PARENTS_SIZE; i++)
    {
        total_fitness += 1.0 / (IDV[i].EVALUATION_VALUE_F_1 + 1);
    }

    double random_value = ((double)rand() / RAND_MAX) * total_fitness;
    double cumulative_fitness = 0;
    for (int i = 0; i < PARENTS_SIZE; i++)
    {
        cumulative_fitness += 1.0 / (IDV[i].EVALUATION_VALUE_F_1 + 1);
        if (cumulative_fitness >= random_value)
        {
            return i;
        }
    }

    return 0;
}

int Optimization::selection_nsga2()
{
    return 0;
}

void Optimization::EvaluationFunction_LineCol(int i)
{

    // 変数の初期化
    IDV[i].EVALUATION_VALUE_F_1 = 0;
    IDV[i].EVALUATION_VALUE_F_2 = 0;

    double score_f_1 = 0;
    double score_f_2 = 0;

    // 違反数最小(f1 : 行違反最小)
    for (int j = 0; j < NUM_LINE; j++)
    {
        IDV[i].EVALUATION_VALUE_LINE[j] = 0;
        long evaluation_f_1 = LONG_MAX;
        for (int k = 0; k < GB.FEASIBLE_LINE[j].size(); k++)
        {
            long current_eval_f_1 = 0;
            for (int l = 0; l < NUM_COL; l++)
            {
                if (IDV[i].BOARD[j][l] != GB.FEASIBLE_LINE[j][k][l])
                {
                    current_eval_f_1++;
                }
            }
            if (current_eval_f_1 < evaluation_f_1)
            {
                evaluation_f_1 = current_eval_f_1;
                IDV[i].EVALUATION_VALUE_LINE[j] = evaluation_f_1;

                if (IDV[i].EVALUATION_VALUE_LINE[j] == 0)
                {
                    break;
                }
            }
        }
        score_f_1 += IDV[i].EVALUATION_VALUE_LINE[j];
    }
    IDV[i].EVALUATION_VALUE_F_1 = score_f_1;

    // 違反数最小(f2 : 列違反最小)
    for (int j = 0; j < NUM_COL; j++)
    {
        IDV[i].EVALUATION_VALUE_COLUMN[j] = 0;
        long evaluation_f_2 = LONG_MAX;
        for (int k = 0; k < GB.FEASIBLE_COLUMN[j].size(); k++)
        {
            long current_eval_f_2 = 0;
            for (int l = 0; l < NUM_LINE; l++)
            {
                if (IDV[i].BOARD[l][j] != GB.FEASIBLE_COLUMN[j][k][l])
                {
                    current_eval_f_2++;
                }
            }
            if (current_eval_f_2 < evaluation_f_2)
            {
                evaluation_f_2 = current_eval_f_2;
                IDV[i].EVALUATION_VALUE_COLUMN[j] = evaluation_f_2;

                if (IDV[i].EVALUATION_VALUE_COLUMN[j] == 0)
                {
                    break;
                }
            }
        }
        score_f_2 += IDV[i].EVALUATION_VALUE_COLUMN[j];
    }
    IDV[i].EVALUATION_VALUE_F_2 = score_f_2;
}

void Optimization::EvaluationFunction_Objective(int i)
{
    // 変数の初期化
    IDV[i].EVALUATION_VALUE_F_1 = 0;
    IDV[i].EVALUATION_VALUE_F_2 = 0;

    double score_f_1 = 0;
    double score_f_2 = 0;

    // 目的関数 f1 の計算
    for (int j = 0; j < NUM_LINE; j++)
    {
        score_f_1 += IDV[i].EVALUATION_VALUE_LINE[j];
    }
    IDV[i].EVALUATION_VALUE_F_1 = score_f_1;

    // 目的関数 f2 の計算
    for (int j = 0; j < NUM_COL; j++)
    {
        score_f_2 += IDV[i].EVALUATION_VALUE_COLUMN[j];
    }
    IDV[i].EVALUATION_VALUE_F_2 = score_f_2;
}
void Optimization::Plot_result(int i, int simu, string filename)
{
    string dir = "results/score/pareto(" +
                 to_string(FIXCELL_FLAG) + "_" +
                 to_string(CROSSOVER_FLAG) + "_" +
                 to_string(PARENTS_FLAG) + "_" +
                 to_string(MUTATION_FLAG) + "_" +
                 to_string(SELECTION_FLAG) + ") + " +
                 to_string(MUTATION_RATE) + "/" +
                 "simu_" + to_string(i) + "/";

    filesystem::create_directories(dir);

    string name_latest = dir + filename + to_string(i) + "_" + to_string(GENERATION_SIZE) + ".csv";
    ofstream file_latest(name_latest);

    if (!(simu == 1 or simu % 50 == 0))
    {
        file_latest << "F1,F2" << endl;
        for (int i = 0; i < PARENTS_SIZE; i++)
        {
            file_latest << IDV[i].EVALUATION_VALUE_F_1 << "," << IDV[i].EVALUATION_VALUE_F_2 << endl;
        }
        return;
    }

    string name = dir + filename + to_string(i) + "_" + to_string(simu) + ".csv";
    ofstream file_write(name);

    file_write << "F1,F2" << endl;
    for (int i = 0; i < PARENTS_SIZE; i++)
    {
        file_write << IDV[i].EVALUATION_VALUE_F_1 << "," << IDV[i].EVALUATION_VALUE_F_2 << endl;
    }
}

void Optimization::BestScore_Result(int simu, int gene)
{
    string dir_simu = "results/score/pareto(" +
                      to_string(FIXCELL_FLAG) + "_" +
                      to_string(CROSSOVER_FLAG) + "_" +
                      to_string(PARENTS_FLAG) + "_" +
                      to_string(MUTATION_FLAG) + "_" +
                      to_string(SELECTION_FLAG) + ") + " +
                      to_string(MUTATION_RATE) + "/" +
                      "simu_" + to_string(simu) + "/";

    filesystem::create_directories(dir_simu);

    string dir_memo = "results/score/pareto(" +
                      to_string(FIXCELL_FLAG) + "_" +
                      to_string(CROSSOVER_FLAG) + "_" +
                      to_string(PARENTS_FLAG) + "_" +
                      to_string(MUTATION_FLAG) + "_" +
                      to_string(SELECTION_FLAG) + ") + " +
                      to_string(MUTATION_RATE) + "/";

    filesystem::create_directories(dir_memo);

    string best_name = dir_simu + "best_score.csv";
    ofstream file_writebest(best_name, ios::app);

    file_writebest << gene << "," << BT_IDV[simu][gene].EVALUATION_VALUE_F_1 << "," << BT_IDV[simu][gene].EVALUATION_VALUE_F_2 << endl;

    if (gene == GENERATION_SIZE - 1 || (BT_IDV[simu][gene].EVALUATION_VALUE_F_1 == 0 && BT_IDV[simu][gene].EVALUATION_VALUE_F_2 == 0))
    {
        string summary_file = dir_memo + "simu.csv";
        ofstream file_write_summary(summary_file, ios::app);
        file_write_summary << gene << "," << BT_IDV[simu][gene].EVALUATION_VALUE_F_1 << "," << BT_IDV[simu][gene].EVALUATION_VALUE_F_2 << endl;

        string board_name = dir_simu + "best_board" + ".csv";
        ofstream board_file(board_name);

        for (int i = 0; i < NUM_LINE; i++)
        {
            for (int j = 0; j < NUM_COL; j++)
            {
                board_file << BT_IDV[simu][gene].BOARD[i][j] << ",";
            }
            board_file << endl;
        }
    }
}