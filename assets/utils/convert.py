input_file_path = 'assets/dining_room/floor_dining.obj'  # 你的输入OBJ文件路径
output_file_path = 'assets/dining_room2/floor_dining.obj'  # 输出OBJ文件路径
# input_file_path = 'assets/meshes/floor.obj'  # 你的输入OBJ文件路径
# output_file_path = 'assets/meshes/floor2.obj'  # 输出OBJ文件路径

import os

def convert(input_file_path, output_file_path):
# 打开输入文件并读取内容
    with open(input_file_path, 'r') as input_file:
        lines = input_file.readlines()

    # 筛选出v和f行，删除f行中的vn信息
    v_lines = [line.strip() for line in lines if line.startswith('v ')]
    f_lines = [line.strip() for line in lines if line.startswith('f ')]
    new_f_lines = []
    for line in f_lines:
        words = line.split()
        new_words = []
        # print(words)
        for word in words:
            new_words = []
            new_word = word.split('/')
            print(new_word)
            if len(new_word) < 3:
                continue
            new_f_lines.append(f"f {new_word[0]} {new_word[1]} {new_word[2]}")

        # new_f_lines.append(f"f {new_words[1]} {new_words[2]} {new_words[3]}")
        if len(new_words) == 4:
            new_f_lines.append(f"f {new_words[1]} {new_words[4]} {new_words[3]}")

    print(new_f_lines)
    # 打开输出文件并写入v和f行
    with open(output_file_path, 'w') as output_file:
        output_file.write('\n'.join(v_lines) + '\n')
        output_file.write('\n'.join(new_f_lines) + '\n')

    print(f"成功创建文件: {output_file_path}")

if __name__ == '__main__':
    # for filename in os.listdir("assets/dining_room"):
    #     if not filename.endswith('.obj'):
    #         continue
    #     input_file_path = "assets/dining_room" + "/" + filename
    #     output_file_path = "assets/dining_room2" + "/" + filename
    #     print(input_file_path)
    #     convert(input_file_path, output_file_path)
    convert(input_file_path, output_file_path)