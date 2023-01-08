from PIL import Image
import numpy as np
import os
import ffmpeg

lines_per_file = 101
smallfile = None
i = 0
with open('bin/rezultati.txt') as bigfile:
    for lineno, line in enumerate(bigfile):
        if lineno % lines_per_file == 0:
            if smallfile:
                smallfile.close()
            i = i + 1
            small_filename = 'img/{}.txt'.format(f'{i:05}')
            smallfile = open(small_filename, "w")
        smallfile.write(line)
    if smallfile:
        smallfile.close()

i = 0
text_img_path = "img/"
for text_img in sorted(os.listdir(text_img_path)):
    i = i + 1
    img = Image.fromarray((np.loadtxt("img/" + text_img) * 55).astype(np.uint8)) 
    img.save("final/{}.png".format(f'{i:05}'), format="png")
