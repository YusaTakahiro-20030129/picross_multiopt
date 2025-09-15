import pandas as pd
import os
import re

# 集計したいトップディレクトリ
top_dir = "score"

# 結合用の空のデータフレーム
merged_df = pd.DataFrame()

# 再帰的に全フォルダを探索
for root, dirs, files in os.walk(top_dir):
    for file in files:
        # ファイル名が simu_X_400.csv の形式にマッチするかチェック
        if re.match(r"simu_\d+_400\.csv$", file):
            file_path = os.path.join(root, file)
            df = pd.read_csv(file_path)
            
            # オプション：どの simu_X 由来か列に追加
            simu_match = re.match(r"(simu_\d+)_400\.csv$", file)
            if simu_match:
                df["simu_folder"] = simu_match.group(1)
            
            merged_df = pd.concat([merged_df, df], ignore_index=True)

# 結合したデータを保存
merged_df.to_csv("merged_simu_400.csv", index=False)
print("CSVを結合して merged_simu_400.csv を作成しました。")