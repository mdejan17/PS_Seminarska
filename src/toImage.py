from PIL import Image
import numpy as np
input = np.loadtxt("rezultati.txt", dtype='i')
print(input)

#data = np.zeros( (1024,1024,3), dtype=np.uint8 )

#data[512,512] = [254,0,0]       # Makes the middle pixel red
#data[512,513] = [0,0,255]       # Makes the next pixel blue
input = (input * 55).astype(np.uint8)
img = Image.fromarray(input)       # Create a PIL image
img.show()   