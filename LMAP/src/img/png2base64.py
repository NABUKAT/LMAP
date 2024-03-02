# -*- coding:utf-8 -*-
from PIL import Image
import base64

filename = "omikuji.png"
savename_png = "omikuji_min.png"
savename_txt = "omikuji_min.png.txt"

# PNG画像の圧縮
img = Image.open(filename)
img = img.convert("P", palette=Image.ADAPTIVE, colors=16)
img.save(savename_png, optimize=True)

# Base64変換
with open(savename_png, 'rb') as f:
    data = f.read()
encode = "data:image/png;base64," + base64.b64encode(data).decode('utf-8')
with open(savename_txt, "w") as f:
    f.write(encode)