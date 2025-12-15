import os
import csv
import numpy as np
import matplotlib.pyplot as plt

# ------------------------------------------------------
# ★ ここだけ書き換えて使う！
#   simu_0 ～ simu_19 が入っている「scoreフォルダの下の実験フォルダ」
# ------------------------------------------------------
TARGET_DIR = "results/score_GUN/score/pareto(1_1_3_4_1) + 0.300000"

TARGET_FILE = "best_board.csv"

def load_board(path):
    """best_board.csv を numpy 配列に変換
       空セルは 0 として扱う"""
    board = []
    with open(path, "r") as f:
        reader = csv.reader(f)
        for row in reader:
            row_clean = []
            for x in row:
                try:
                    row_clean.append(int(x))
                except ValueError:
                    # 空文字や数値以外は 0 とする
                    row_clean.append(0)
            board.append(row_clean)
    return np.array(board)


def plot_board(board, save_path):
    """白=0、黒=1 の盤面として描画して保存"""
    plt.figure(figsize=(5, 5))
    plt.imshow(board, cmap="gray_r", interpolation="nearest")
    plt.xticks([])
    plt.yticks([])
    plt.title(os.path.basename(os.path.dirname(save_path)))
    plt.savefig(save_path, dpi=300, bbox_inches="tight")
    plt.close()
    print("Saved:", save_path)


def main():
    # simu_0 ～ simu_19 を探索
    simu_dirs = sorted(
        [os.path.join(TARGET_DIR, d) for d in os.listdir(TARGET_DIR)
         if d.startswith("simu_") and os.path.isdir(os.path.join(TARGET_DIR, d))]
    )

    if len(simu_dirs) == 0:
        print("simu_* が見つかりませんでした。")
        print("確認したフォルダ:", TARGET_DIR)
        return

    print(f"発見した simu_* : {len(simu_dirs)} 個")

    for simu in simu_dirs:
        board_csv_path = os.path.join(simu, TARGET_FILE)

        if not os.path.isfile(board_csv_path):
            print("スキップ（best_board.csv なし）:", simu)
            continue

        board = load_board(board_csv_path)

        output_png = os.path.join(simu, "best_board.png")
        plot_board(board, output_png)

    print("\n完了！ 全ての best_board.png を出力しました。\n")


if __name__ == "__main__":
    main()