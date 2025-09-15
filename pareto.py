import pandas as pd
import matplotlib.pyplot as plt
import os

score_pynum = 1  # シミュレーション回数の参照番号
generation_size = 400  # C++のGENERATION_SIZEと一致させる
simuration_size = 20;  # C++のSIMURATION_SIZEと一致させる

# プロット図を見たいならば1にする
flg_show = 0

# チェック候補の算出(パレート出力区間の設定)
check_start_num = 0 
check_simu_num = 20 

# Parameters.cppの番号と一致させる
FIXCELL_FLAG = 1
CROSSOVER_FLAG = 1
PARENTS_FLAG = 2
MUTATION_FLAG = 1
SELECTION_FLAG = 1

##### ここから下は触らない！！！ #####
size = generation_size / 50 + 1
plot_size = int(size)

score_gene = [50 * i for i in range(plot_size)]

for h in range(check_start_num, check_simu_num):
    score_pynum = h
    for i in range(plot_size):
        filename_csv = f"results/score/pareto({FIXCELL_FLAG}_{CROSSOVER_FLAG}_{PARENTS_FLAG}_{MUTATION_FLAG}_{SELECTION_FLAG})/simu_{h}/pareto_{score_pynum}_{score_gene[i]}.csv"
        filename_fig = f"results_fig/score/pareto({FIXCELL_FLAG}_{CROSSOVER_FLAG}_{PARENTS_FLAG}_{MUTATION_FLAG}_{SELECTION_FLAG})/simu_{h}/pareto_{score_pynum}pareto_gene_{score_gene[i]}.png"

        # ファイル存在チェック（無ければスキップ）
        if not os.path.exists(filename_csv):
            continue

        # ディレクトリ作成
        os.makedirs(os.path.dirname(filename_fig), exist_ok=True)

        df = pd.read_csv(filename_csv)

        # 偶数番目と奇数番目に分割
        even_df = df.iloc[::2].copy()
        odd_df = df.iloc[1::2].copy()

        # Setに変換して重複 (F1, F2) を検出
        even_set = set([tuple(x) for x in even_df[["F1", "F2"]].values])
        odd_set = set([tuple(x) for x in odd_df[["F1", "F2"]].values])
        overlap_set = even_set & odd_set  # 共通部分（重複点）

        # 重複データのみ抽出
        overlap_df = pd.DataFrame(list(overlap_set), columns=["F1", "F2"])

        # 重複以外をフィルタリングして再作成
        even_unique = even_df[~even_df[["F1", "F2"]].apply(tuple, axis=1).isin(overlap_set)]
        odd_unique = odd_df[~odd_df[["F1", "F2"]].apply(tuple, axis=1).isin(overlap_set)]

        # プロット
        plt.figure(figsize=(8,6))
        plt.scatter(even_unique["F1"], even_unique["F2"], c="green", label="F1pick")
        plt.scatter(odd_unique["F1"], odd_unique["F2"], c="blue", label="F2pick")
        if not overlap_df.empty:
            plt.scatter(overlap_df["F1"], overlap_df["F2"], c="red", label="F1 & F2")

        plt.xlabel("Line_illigal")
        plt.ylabel("Column_illigal")
        plt.title(f"Picross Score @ generation {score_gene[i]}")

        plt.xlim([0, max(df["F1"]) + 10])
        plt.ylim([0, max(df["F2"]) + 10])
        plt.legend()
        plt.grid(True)
        plt.savefig(filename_fig)
            
        plt.close()


