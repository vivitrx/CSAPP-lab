import sys

# 检查命令行参数
if len(sys.argv) != 3:
    print("用法: python3 delete_lines.py <源文件> <目标文件>")
    sys.exit(1)

# 从命令行获取源文件和目标文件
input_file = sys.argv[1]
output_file = sys.argv[2]

try:
    # 读取源文件内容
    with open(input_file, "r", encoding="utf-8") as file:
        lines = file.readlines()

    # 第一次删除操作：删除第1行和每隔4行的一行
    filtered_lines_step1 = [
        line for i, line in enumerate(lines)
        if (i != 0 and (i % 4 != 0))
    ]

    # 第二次删除操作：删除第1行和每隔3行的一行（基于第一次删除的结果）
    filtered_lines_step2 = [
        line for i, line in enumerate(filtered_lines_step1)
        if (i != 0 and (i % 3 != 0))
    ]

    # 将结果写入目标文件
    with open(output_file, "w", encoding="utf-8") as file:
        file.writelines(filtered_lines_step2)

    print(f"已成功处理文件，并将结果保存到: {output_file}")

except FileNotFoundError:
    print(f"错误: 文件 {input_file} 不存在，请检查路径。")
except Exception as e:
    print(f"发生错误: {e}")
