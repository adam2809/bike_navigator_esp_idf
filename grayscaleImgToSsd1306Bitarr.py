import cv2
import numpy as np
import sys

def grayscale_to_negated_bitarr(img_arr):
    return np.vectorize(lambda x: 0 if x == 255 else 1)(img_arr)
    
def display_img(img,title):
    print(f"{title}: {img}")
    cv2.imshow(title,img)
    k = cv2.waitKey(0)
    if k == 27:
        cv2.destroyAllWindows()

def bit_arr_to_int(bit_arr):
    if len(bit_arr) != 8:
        sys.exit(f"Bit array must be 8 long (is {len(bit_arr)})")
    res = 0
    for i,bit in enumerate(bit_arr):
        res += bit * (2**i)
    return res


def get_bit_arr_str_from_img(img_arr):
    img_arr = grayscale_to_negated_bitarr(img_arr)[:,::-1]
    width  = img_arr.shape[1]
    height  = img_arr.shape[0]

    if(width % 8) != 0:
        sys.exit("The image width has to be divisible by 8")

    res = ''
    res += '{\n'
    for i in range(width//8):
        page = '\t{'
        for j in range(height):
            page+= hex(bit_arr_to_int(img_arr[j,i*8:(i+1)*8]))+','
        page = page[:-1]
        page += '}'
        page += ',' if i != 7 else ''
        res += page+"\n"
    res += '}\n'

    return res


if __name__ == '__main__':
    if(len(sys.argv) != 2):
        sys.exit(f"Wrong amount of command line arguments (is {len(sys.argv)-1} should be 1)")
    
    img = cv2.imread(sys.argv[1],0)

    if(img is None):
        sys.exit(f"Image with name {sys.argv[1]} does not exist")

    print(get_bit_arr_str_from_img(img))

