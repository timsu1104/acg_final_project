import cv2
import os
import argparse
import re

parent_folder = "E:/YaoClass/2023Fall/Advanced Computer Graphics/project/acg_final_project/build/src/sparks"
video_name = "output_video.avi"

def natural_sort_key(s):
    # 将数字提取出来，用于自然排序
    return [int(text) if text.isdigit() else text.lower() for text in re.split(r'(\d+)', s)]

if __name__ == '__main__':
    
    parser = argparse.ArgumentParser()

    parser.add_argument("image_folder", type=str)
    args = parser.parse_args()


    image_folder = os.path.join(parent_folder, args.image_folder)

    images = [img for img in os.listdir(image_folder) if img.endswith(".png") or img.endswith(".jpg")]

    images = sorted(images, key=natural_sort_key)

    frame = cv2.imread(os.path.join(image_folder, images[0]))
    height, width, layers = frame.shape

    video = cv2.VideoWriter(os.path.join(image_folder, video_name), cv2.VideoWriter_fourcc(*"DIVX"), 30, (width, height))

    for image in images:
        video.write(cv2.imread(os.path.join(image_folder, image)))

    video.release()
