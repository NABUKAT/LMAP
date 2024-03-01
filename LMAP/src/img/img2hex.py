# -*- coding:utf-8 -*-
import numpy as np
from PIL import Image
import glob
import os

IMG_FOLDER_PATH = "./org/*"
SAVE_FILE_PATH  = "./ImgData.h.txt"

# RGB値を16bit(RGB565)のテキストに変換する
def rgb2hexstr(rgb):
    col = ((rgb[0]>>3)<<11) | ((rgb[1]>>2)<<5) | (rgb[2]>>3)
    return "0x{:04X}".format(col)

# 16bit(RGB565)のRGB画像HEX出力
def outputColorPixel(width, height, image):
    result_str = ""

    # パレット読み込み
    if image.mode == 'P':
        palette = np.array(image.getpalette()).reshape(-1, 3)  # n行3列に変換
        getPixel = lambda x,y: palette[image.getpixel((x, y))]
    else:
        getPixel = lambda x,y: image.getpixel((x, y))

    # HEX出力
    for y in range(height):
        x_cnt = 0
        for x in range(width):
            if x_cnt == 0:
                result_str += "    "
            pixel = getPixel(x, y)
            result_str += rgb2hexstr(pixel) + ","
            x_cnt = x_cnt + 1
            if x_cnt >= width:
                x_cnt = 0
                result_str += "\n"

    return result_str[:-2] + "\n"

if __name__ == '__main__':
    # 画像リストを取得
    img_list = sorted(glob.glob(IMG_FOLDER_PATH))

    # 画像がない場合は終了
    if len(img_list) == 0:
        print("No image File!")
        exit()
    
    f = open(SAVE_FILE_PATH, 'w')

    header_str = ""
    fcnt = 0
    # 画像の情報を書き込む
    for fn in img_list:
        # 画像ファイル読み込み
        image = Image.open(fn)
        width, height = image.size

        # 画像ファイル名取得
        fn_o_u = os.path.splitext(os.path.basename(fn))[0].upper()

        # サイズを定義する
        #header_str += "#define IMG_" + fn_o_u + "_W " + str(width) +"\n"
        #header_str += "#define IMG_" + fn_o_u + "_H " + str(height) +"\n"
        fcnt += 1
    header_str += "\n"
    f.write(header_str)

    fcnt = 0
    for fn in img_list:
        fcnt += 1

        # 画像ファイル読み込み
        image = Image.open(fn)
        width, height = image.size

        # 画像ファイル名取得
        fn_o_u = os.path.splitext(os.path.basename(fn))[0].upper()

        # 画像変数定義部分
        header_str = "const unsigned short IMG_" + fn_o_u + "[] PROGMEM = {\n"
        f.write(header_str)

        # 画像の色配列情報の文字列を取得して書き込む
        image_hex = outputColorPixel(width, height, image)
        f.write(image_hex)

        f.write("};\n")

    # 定義ファイルを書き込んで閉じる
    f.close()

    print("saved " + SAVE_FILE_PATH)