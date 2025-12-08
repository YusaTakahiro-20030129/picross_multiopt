import os
import csv
import matplotlib.pyplot as plt

# ---------------------------------------------------
#  ★ ここで対象フォルダを指定する ★
#  出力ファイルもここに保存される
# ---------------------------------------------------
TARGET_DIR = "results/score_SPLAY/score/pareto(1_1_3_4_1) + 0.100000"
TARGET_FILE = "best_score.csv"


def find_csv_files(target_dir, target_name):
    """指定されたディレクトリ内の best_score.csv を全部探す。"""
    csv_paths = []
    for root, dirs, files in os.walk(target_dir):
        if target_name in files:
            csv_paths.append(os.path.join(root, target_name))
    return csv_paths


def load_best_score(path):
    gens, f1s, f2s = [], [], []
    with open(path, "r") as f:
        reader = csv.reader(f)
        for row in reader:
            if len(row) < 3:
                continue
            gens.append(int(row[0]))
            f1s.append(float(row[1]))
            f2s.append(float(row[2]))
    return gens, f1s, f2s


def save_plot(fig, filename):
    """TARGET_DIR に保存する"""
    output_path = os.path.join(TARGET_DIR, filename)
    fig.savefig(output_path, dpi=300)
    print("Saved:", output_path)


def main():

    csv_files = find_csv_files(TARGET_DIR, TARGET_FILE)

    if len(csv_files) == 0:
        print("best_score.csv が見つかりませんでした。")
        print(f"検索対象フォルダ: {TARGET_DIR}")
        return

    print(f"見つかった CSV: {len(csv_files)} 個")
    print("対象ディレクトリ:", TARGET_DIR)

    # ------------------- F1 曲線 -------------------
    fig = plt.figure()
    for path in csv_files:
        gens, f1s, f2s = load_best_score(path)
        plt.plot(gens, f1s, alpha=0.5)
    plt.xlabel("Generation")
    plt.ylabel("F1 Score")
    plt.title("Learning Curve — F1")
    plt.grid()
    plt.tight_layout()
    save_plot(fig, "learning_curve_f1.png")

    # ------------------- F2 曲線 -------------------
    fig = plt.figure()
    for path in csv_files:
        gens, f1s, f2s = load_best_score(path)
        plt.plot(gens, f2s, alpha=0.5)
    plt.xlabel("Generation")
    plt.ylabel("F2 Score")
    plt.title("Learning Curve — F2")
    plt.grid()
    plt.tight_layout()
    save_plot(fig, "learning_curve_f2.png")

    # ----------- ユークリッド距離曲線 -----------
    fig = plt.figure()
    for path in csv_files:
        gens, f1s, f2s = load_best_score(path)
        dist = [(a*a + b*b) ** 0.5 for a, b in zip(f1s, f2s)]
        plt.plot(gens, dist, alpha=0.5)
    plt.xlabel("Generation")
    plt.ylabel("Euclidean Distance")
    plt.title("Learning Curve — Euclid")
    plt.grid()
    plt.tight_layout()
    save_plot(fig, "learning_curve_euclid.png")

    print("\n完了！ 生成した画像は TARGET_DIR に保存されました。\n")


if __name__ == "__main__":
    main()