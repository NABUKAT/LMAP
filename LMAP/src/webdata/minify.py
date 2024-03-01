# -*- coding:utf-8 -*-
import re
import os
import shutil

src = "src"
dest = "../../data"

# 最小化処理
def minify(src_path, dest_path):
    with open(src_path, encoding="utf_8") as f:
        s0 = f.readlines()
        # JS用1行コメント(//以降)を削除
        s1 = []
        for s in s0:
            nocom = False
            s_cnt = 0
            ind = 0
            c_ind = -1
            for c in list(s):
                if nocom == False and (c == "'" or c == '"'):
                    nocom = True
                elif nocom == True and (c == "'" or c == '"'):
                    nocom = False
                if nocom == False and c == "/":
                    if s_cnt == 0:
                        s_cnt = s_cnt + 1
                    elif s_cnt == 1:
                        c_ind = ind - 1
                        break
                elif nocom == False and c != "/":
                    s_cnt = 0
                ind = ind + 1
            if c_ind != -1:
                s1.append(s[:c_ind])
            else:
                s1.append(s)
        # 各行の前後の空白を削除
        s2 = "".join([s.strip() for s in s1])
        # コメント(/* */)の削除
        s3 = re.sub('/\*.*?\*/', '', s2)
        # HTML用コメントの削除
        s4 = re.sub('<!--.*?-->', '', s3)
        # CSS用
        s5 = re.sub(': +?', ':', s4) # セミコロン後の空白を削除
        s6 = re.sub(' +?{', '{', s5) # '{'前の空白を削除
    # ファイル書き込み
    with open(dest_path, mode='w', encoding="utf_8") as f:
        f.write(s6)
    # サイズと削減率を表示
    s_size = os.path.getsize(src_path)
    d_size = os.path.getsize(dest_path)
    reduction = s_size - d_size
    r_per = reduction / s_size
    print("---")
    print("*" + os.path.basename(src_path))
    print(" src_size: " + '{:,}'.format(s_size) + " B")
    print(" dst_size: " + '{:,}'.format(d_size) + " B")
    print(" reduction: " + '{:,}'.format(reduction) + " B (" + '{:.1f}'.format(r_per*100) + " %)")

# 対象ディレクトリ内の各ファイル最小化
for current_dir, sub_dirs, files_list in os.walk(src):
    # サブディレクトリを取得し、出力フォルダに作成
    for subdir in sub_dirs:
        os.makedirs(dest+"/"+subdir, exist_ok=True)

    # 最小化可能なファイルを最小化
    for file in files_list:
        extention = file.split(".")[-1] # 拡張子
        file_path = current_dir+"/"+file
        if extention in ["html", "css", "js"]:
            minify(file_path, file_path.replace(src, dest, 1))
        else:
            shutil.copy2(file_path, file_path.replace(src, dest, 1))
print("---")