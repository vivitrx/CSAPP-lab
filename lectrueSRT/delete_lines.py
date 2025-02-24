import sys
import re

# 检查命令行参数
if len(sys.argv) != 3:
    print("用法: python3 delete_lines.py <源文件> <目标文件>")
    sys.exit(1)

# 获取文件名
input_file = sys.argv[1]
output_file = sys.argv[2]

def is_english(text):
    """判断一行是否主要是英文"""
    return re.match(r'^[A-Za-z0-9\s\.,!?\'"-]+$', text.strip()) is not None

try:
    # 读取源文件
    with open(input_file, "r", encoding="utf-8") as file:
        lines = file.readlines()

    # 过滤掉英文行，并去除空行
    chinese_lines = [line.strip() for line in lines if not is_english(line) and line.strip()]

    # 让中文字幕之间保持一行间隔
    spaced_lines = "\n\n".join(chinese_lines) + "\n"

    # 写入目标文件
    with open(output_file, "w", encoding="utf-8") as file:
        file.write(spaced_lines)

    print(f"已成功处理文件，并将结果保存到: {output_file}")

except FileNotFoundError:
    print(f"错误: 文件 {input_file} 不存在，请检查路径。")
except Exception as e:
    print(f"发生错误: {e}")
