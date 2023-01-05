from PIL import Image
import numpy as np

img = Image.fromarray((np.loadtxt("rezultati.txt", dtype='i') * 55).astype(np.uint8)) 
img.show()   