import cv2
import numpy as np
import sys
from os import walk

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
    res += '}'

    return res

def get_nums_arr_of_bitarrs():
    nums = []

    nums.append(cv2.imread("res/numbers/zero.png",0))
    nums.append(cv2.imread("res/numbers/one.png",0))
    nums.append(cv2.imread("res/numbers/two.png",0))
    nums.append(cv2.imread("res/numbers/three.png",0))
    nums.append(cv2.imread("res/numbers/four.png",0))
    nums.append(cv2.imread("res/numbers/five.png",0))
    nums.append(cv2.imread("res/numbers/six.png",0))
    nums.append(cv2.imread("res/numbers/seven.png",0))
    nums.append(cv2.imread("res/numbers/eight.png",0))
    nums.append(cv2.imread("res/numbers/nine.png",0))

    res = "{\n"
    for n in nums:
        res+=get_bit_arr_str_from_img(n)+","
    res=res[:-1]
    res+="\n}"
    return res

MANEUVERS_PATH = "res/maneuvers";
NUMBERS_PATH = "res/numbers";

if __name__ == '__main__':
    print("uint8_t numbers[DIGIT_COUNT][NUMBER_PAGE_COUNT][NUMBER_WIDTH] ="+get_nums_arr_of_bitarrs()+";\n\n")

    for (curr_dirr,_, filenames) in walk('res'):
        page_count = "";
        width = "";
        if curr_dirr == NUMBERS_PATH:
            continue

        if curr_dirr == MANEUVERS_PATH:
            page_count = "MANEUVER_PAGE_COUNT"
            width = "MANEUVER_WIDTH"

        for fn in filenames:
            if fn in ['greater_than.png','km.png']:
                page_count = "NUMBER_PAGE_COUNT"
                width = "NUMBER_WIDTH"

            if fn == 'bt_icon.png':
                page_count = "BT_ICON_PAGE_COUNT"
                width = "BT_ICON_WIDTH"

            if page_count == "" or width == "":
                print(f"Unset width or page count skipping {fn}")
                continue

            img = cv2.imread(curr_dirr+'/'+fn,0)
            print(f"uint8_t {fn[:fn.find('.')]}[{page_count}][{width}] = {get_bit_arr_str_from_img(img)};\n\n")