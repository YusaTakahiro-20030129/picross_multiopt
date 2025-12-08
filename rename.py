import os

# 対象の実験フォルダ
BASE_DIR = "results/score_SPLAY/score/pareto(1_1_3_4_1) + 0.100000"

# 統一するファイル名
UNIFIED_NAME = "best_board.csv"

# simu_0 ～ simu_19 を探索
for simu in os.listdir(BASE_DIR):
    simu_path = os.path.join(BASE_DIR, simu)
    if os.path.isdir(simu_path) and simu.startswith("simu_"):
        # フォルダ内の best_board_*.csv を検索
        for file_name in os.listdir(simu_path):
            if file_name.startswith("best_board_") and file_name.endswith(".csv"):
                old_file = os.path.join(simu_path, file_name)
                new_file = os.path.join(simu_path, UNIFIED_NAME)
                
                # 上書きする場合は注意
                if os.path.exists(new_file):
                    os.remove(new_file)
                
                os.rename(old_file, new_file)
                print(f"Renamed: {old_file} -> {new_file}")

print("完了！指定のファイル名に統一しました。")