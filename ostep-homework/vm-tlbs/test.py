import subprocess
import os
import re
import matplotlib.pyplot as plt

def run_cpp_executable(executable_path, args=None):
    """
    运行指定的C++二进制程序，并打印其输出。
    args: 一个列表，包含要传递给C++程序的命令行参数。
    Returns: 如果成功，返回解析出的平均访问时间 (float)；否则返回 None。
    """
    if args is None:
        args = []

    command = [executable_path] + [str(arg) for arg in args]
    print(f"尝试运行命令: {' '.join(command)}")

    if not os.path.exists(executable_path):
        print(f"错误: 找不到指定的二进制程序 '{executable_path}'。")
        print("请确保路径正确，并且程序已经编译成功。")
        return None # 返回None表示运行失败

    # 确保文件是可执行的（在Linux/macOS上）
    if os.name != 'nt' and not os.access(executable_path, os.X_OK):
        print(f"警告: 文件 '{executable_path}' 没有执行权限。尝试添加执行权限...")
        try:
            os.chmod(executable_path, os.stat(executable_path).st_mode | 0o111)
            print("已添加执行权限。")
        except Exception as e:
            print(f"无法添加执行权限: {e}")
            print("请手动确保程序有执行权限 (例如: chmod +x your_executable_name)。")
            return None

    try:
        # 运行程序并捕获其输出
        result = subprocess.run(command, capture_output=True, text=True, check=True)

        print("\n--- 程序输出 ---")
        print(result.stdout)
        if result.stderr:
            print("--- 程序错误输出 (stderr) ---")
            print(result.stderr)
        print("--- 程序运行结束 ---")

        # 解析C++程序的输出以提取 "ns/page" 的值
        # 新的正则表达式匹配 "X.X ns/page" 格式
        match = re.search(r"(\d+\.?\d*)\sns/page", result.stdout)
        if match:
            avg_time_per_page = float(match.group(1))
            print(f"解析到每次访问平均时间 (ns/page): {avg_time_per_page} ns")
            return avg_time_per_page
        else:
            print("警告: 未能在C++程序输出中找到 'X.X ns/page'。请检查C++程序的输出格式。")
            return None

    except subprocess.CalledProcessError as e:
        print(f"\n错误: 程序 '{executable_path}' 运行失败，返回码: {e.returncode}")
        print(f"标准输出:\n{e.stdout}")
        print(f"标准错误:\n{e.stderr}")
        return None
    except Exception as e:
        print(f"\n运行程序时发生未知错误: {e}")
        return None

NUM_TRIALS = 16384 # 假设这是你的C++程序需要的第二个参数，例如迭代次数
MAX_PAGES = 16384 # 最大页数

if __name__ == "__main__":
    # 请在这里修改为你的C++二进制程序的路径和名称
    # 例如: "./my_tlb_test" 或 "C:\\Users\\YourUser\\Documents\\my_tlb_test.exe"
    your_executable_name = "./tlb" # 假设你的可执行文件名为 tlb 并且在当前目录

    # 定义要测试的页数范围
    num_pages_to_test = []
    current_pages = 1
    while current_pages <= MAX_PAGES:
        num_pages_to_test.append(current_pages)
        current_pages *= 2
    
    print("将测试的页数:", num_pages_to_test)
    print("\n" + "="*50 + "\n")

    # 用于存储绘图数据
    pages_data = []
    avg_times_data = []

    for pages in num_pages_to_test:
        print(f"=== 正在测试访问 {pages} 页的情况 ===")
        # 将页数和试次数作为参数传递给C++程序
        # 假设你的C++程序接受两个参数：第一个是页数，第二个是试次数
        avg_time = run_cpp_executable(your_executable_name, args=[pages, NUM_TRIALS])
        
        if avg_time is not None:
            pages_data.append(pages)
            avg_times_data.append(avg_time)
        else:
            print(f"跳过对 {pages} 页的绘图，因为程序运行失败或未解析到数据。")
        print("\n" + "="*50 + "\n")

    print("所有测试完成。")

    # 绘制趋势图
    if pages_data and avg_times_data:
        print("\n--- 正在生成趋势图 ---")
        plt.figure(figsize=(12, 6)) # 设置图表大小
        plt.plot(pages_data, avg_times_data, marker='o', linestyle='-', color='b')
        plt.xscale('log', base=2) # X轴使用对数刻度（以2为底），因为页数是2的幂次增长
        plt.xlabel("Pages", fontsize=12)
        plt.ylabel("Average Access Time (ns/page)", fontsize=12) # 更新Y轴标签
        plt.title("Memory Access Time", fontsize=14)
        plt.grid(True, which="both", ls="--", c='0.7') # 显示网格
        plt.xticks(num_pages_to_test, [str(p) for p in num_pages_to_test], rotation=45, ha='right') # 设置X轴刻度标签
        plt.tight_layout() # 调整布局以避免标签重叠
        
        # 将图表保存为图片文件
        image_filename = "result.png"
        plt.savefig(image_filename)
        print(f"趋势图已保存为 '{image_filename}'。")
        
    else:
        print("没有足够的数据来生成趋势图。")

